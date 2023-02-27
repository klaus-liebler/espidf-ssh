#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "config.h"
#include <libssh/callbacks.h>
#include <libssh/server.h>
#include <poll.h>
#include <sys/queue.h>
#include "example.h"
#include "sshd.h"
#include "esp_log.h"
#include <esp_vfs.h>
#define TAG "SSHD"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define KEYS_FOLDER "/ssh"


static void handle_char_from_local(struct interactive_session *, char);

struct ssh_poll_handle_struct *ssh_bind_get_poll(struct ssh_bind_struct *);
int ssh_event_add_poll(ssh_event event, struct ssh_poll_handle_struct *);
int ssh_event_remove_poll(ssh_event event, struct ssh_poll_handle_struct *);

static size_t sshfs_hostkey_offset = 0;

static ssize_t sshfs_read(void *ctx, int fd, void *dst, size_t size)
{
    if (fd == 10)
    {
    	ESP_LOGI(TAG, "sshfs_read for fd=%i with size=%d called. Current offset=%d", fd, size, sshfs_hostkey_offset);
        memcpy(dst, ctx + sshfs_hostkey_offset, size);
		sshfs_hostkey_offset+=size;
        return size;
    }
    return -1;
}
static int sshfs_open(void *ctx, const char *path, int flags, int mode)
{
    if (!strcmp("/hostkey", path))
    {
        ESP_LOGI(TAG, "vfs_open for hostkey called");
        sshfs_hostkey_offset = 0;
        return 10;
    }
    ESP_LOGW(TAG, "vfs_open for unknown file %s called", path);
    return -1;
}

static int sshfs_close(void *ctx, int fd)
{
    ESP_LOGI(TAG, "vfs_close called for fd=%i", fd);
    sshfs_hostkey_offset = 0;
    return 0;
}
static int sshfs_fstat(void *ctx, int fd, struct stat *st)
{
    
    if (fd == 10){
        size_t len= strlen((const char *)ctx);
        ESP_LOGI(TAG, "vfs_fstat, called for fd=%i, return strlen %d", fd, len);
		st->st_size=len;
        return 0;
    }
    ESP_LOGI(TAG, "vfs_fstat, called for fd=%i", fd);
    return -1;
}

void sshd_prepare_vfs(ssh_bind sshbind, const char *base64_key)
{
    esp_vfs_t myfs = {
        .flags = ESP_VFS_FLAG_CONTEXT_PTR,
        .write_p = NULL,
        .open_p = &sshfs_open,
        .fstat_p = &sshfs_fstat,
        .close_p = &sshfs_close,
        .read_p = &sshfs_read,
    };

    ESP_ERROR_CHECK(esp_vfs_register("/ssh", &myfs, base64_key));
}

static struct client_ctx *
lookup_client(struct server_ctx *sc, ssh_session session)
{
	struct client_ctx *ret;

	SLIST_FOREACH(ret, &sc->sc_client_head, cc_client_list) {
		if (ret->cc_session == session)
			return ret;
	}

	return NULL;
}

static int
auth_password(ssh_session session, const char *user, const char *password,
	      void *userdata)
{
	struct server_ctx *sc = (struct server_ctx *)userdata;
	struct client_ctx *cc;

	cc = lookup_client(sc, session);
	if (cc == NULL)
		return SSH_AUTH_DENIED;
	if (cc->cc_didauth)
		return SSH_AUTH_DENIED;
	if (strcmp("user", user)!=0 && strcmp("password", password) != 0)
		return SSH_AUTH_DENIED;
	cc->cc_didauth = true;

	return SSH_AUTH_SUCCESS;
}



static int
data_function(ssh_session session, ssh_channel channel, void *data, uint32_t len,
	      int is_stderr, void *userdata)
{
	struct client_ctx *cc = (struct client_ctx *)userdata;
	int i;
	char c;
	for (i = 0; i < len; i++) {
		c = ((char*)data)[i];
		if (c == 0x4) /* ^D */ {
			ssh_channel_send_eof(channel);
			ssh_channel_close(channel);
			return len;
		}
		cc->cc_is.is_handle_char_from_remote(&cc->cc_is, c);
	}
	return len;
}

static int
pty_request(ssh_session session, ssh_channel channel, const char *term,
	    int cols, int rows, int py, int px, void *userdata) {
	struct client_ctx *cc = (struct client_ctx *)userdata;

	if (cc->cc_didpty)
	    return SSH_ERROR;
	cc->cc_cols = cols;
	cc->cc_rows = rows;
	cc->cc_px = px;
	cc->cc_py = py;
	strlcpy(cc->cc_term, term, sizeof(cc->cc_term));
	cc->cc_didpty = true;

	return SSH_OK;
}

static int
shell_request(ssh_session session, ssh_channel channel, void *userdata)
{
	struct client_ctx *cc = (struct client_ctx *)userdata;
	if (cc->cc_didshell)
	    return SSH_ERROR;
	cc->cc_didshell = true;
	cc->cc_is.is_handle_char_from_local = handle_char_from_local;
	cc->cc_begin_interactive_session(&cc->cc_is);
	return SSH_OK;
}

static int exec_request(ssh_session session, ssh_channel channel,
                        const char *command, void *userdata)
{
	struct client_ctx *cc = (struct client_ctx *)userdata;
	if (cc->cc_didshell)
	    return SSH_ERROR;
	cc->cc_is.is_handle_char_from_local = handle_char_from_local;
	minicli_handle_command(&cc->cc_is, command);
	ssh_channel_send_eof(channel);
	ssh_channel_close(channel);
	return SSH_OK;
}

static int
pty_resize(ssh_session session, ssh_channel channel, int cols,
                      int rows, int py, int px, void *userdata)
{
	struct client_ctx *cc = (struct client_ctx *)userdata;

	cc->cc_cols = cols;
	cc->cc_rows = rows;
	cc->cc_px = px;
	cc->cc_py = py;

	return SSH_OK;
}

static ssh_channel
channel_open(ssh_session session, void *userdata)
{
	struct server_ctx *sc = (struct server_ctx *)userdata;
	struct client_ctx *cc;

	cc = lookup_client(sc, session);
	if (cc == NULL)
		return NULL;
	if (cc->cc_didchannel)
		return NULL;
	cc->channel_cb = (struct ssh_channel_callbacks_struct) {
		.channel_data_function = data_function,
		.channel_exec_request_function = exec_request,
		.channel_pty_request_function = pty_request,
		.channel_pty_window_change_function = pty_resize,
		.channel_shell_request_function = shell_request,
		.userdata = cc
	};
	cc->cc_channel = ssh_channel_new(session);
	ssh_callbacks_init(&cc->channel_cb);
	ssh_set_channel_callbacks(cc->cc_channel, &cc->channel_cb);
	cc->cc_didchannel = true;

	return cc->cc_channel;
}

static void
incoming_connection(ssh_bind sshbind, void *userdata)
{
	struct server_ctx  *sc = (struct server_ctx *)userdata;
	long t = 0;
	struct client_ctx *cc = calloc(1, sizeof(struct client_ctx));

	cc->cc_session = ssh_new();
	if (ssh_bind_accept(sshbind, cc->cc_session) == SSH_ERROR) {
		goto cleanup;
	}
	cc->cc_begin_interactive_session = sc->sc_begin_interactive_session;
	ssh_set_callbacks(cc->cc_session, &sc->sc_generic_cb);
	ssh_set_server_callbacks(cc->cc_session, &sc->sc_server_cb);
	ssh_set_auth_methods(cc->cc_session, sc->sc_auth_methods);
	ssh_set_blocking(cc->cc_session, 0);
	(void) ssh_options_set(cc->cc_session, SSH_OPTIONS_TIMEOUT, &t);
	(void) ssh_options_set(cc->cc_session, SSH_OPTIONS_TIMEOUT_USEC, &t);

	if (ssh_handle_key_exchange(cc->cc_session) == SSH_ERROR) {
		ssh_disconnect(cc->cc_session);
		goto cleanup;
	}
	/*
	 * Since we set the socket to non-blocking already,
	 * ssh_handle_key_exchange will return SSH_AGAIN.
	 * Start polling the socket and let the main loop drive the kex.
	 */
	SLIST_INSERT_HEAD(&sc->sc_client_head, cc, cc_client_list);
	ssh_event_add_session(sc->sc_sshevent, cc->cc_session);
	return;
cleanup:
	ssh_free(cc->cc_session);
	free(cc);
}

static void
dead_eater(struct server_ctx *sc)
{
	struct client_ctx *cc;
	struct client_ctx *cc_removed = NULL;
	int status;

	SLIST_FOREACH(cc, &sc->sc_client_head, cc_client_list) {
		if (cc_removed) {
			free(cc_removed);
			cc_removed = NULL;
		}
		status = ssh_get_status(cc->cc_session);
		if (status & (SSH_CLOSED | SSH_CLOSED_ERROR)) {
			if (cc->cc_didchannel) {
				ssh_channel_free(cc->cc_channel);
			}
			ssh_event_remove_session(sc->sc_sshevent, cc->cc_session);
			ssh_free(cc->cc_session);
			SLIST_REMOVE(&sc->sc_client_head, cc, client_ctx, cc_client_list);
			cc_removed = cc;
		}
	}
	if (cc_removed) {
		free(cc_removed);
		cc_removed = NULL;
	}
}

static int
create_new_server(struct server_ctx *sc, const char *hardcoded_example_host_key)
{
	SLIST_INIT(&sc->sc_client_head);
	sc->sc_server_cb = (struct ssh_server_callbacks_struct){
		.userdata = sc,
		.auth_password_function = auth_password,
		.channel_open_request_session_function = channel_open
	};
	sc->sc_generic_cb = (struct ssh_callbacks_struct){
		.userdata = sc
	};
	sc->sc_bind_cb = (struct ssh_bind_callbacks_struct){
		.incoming_connection = incoming_connection
	};
	ssh_callbacks_init(&sc->sc_server_cb);
	ssh_callbacks_init(&sc->sc_generic_cb);
	ssh_callbacks_init(&sc->sc_bind_cb);
    
    sshd_prepare_vfs(sc->sc_sshbind, hardcoded_example_host_key);
    
    ESP_LOGI(TAG, "Calling ssh_bind_new()");
	sc->sc_sshbind = ssh_bind_new();
	if (sc->sc_sshbind == NULL) {
		ESP_LOGE(TAG, "sc->sc_sshbind == NULL");
		return SSH_ERROR;
	}
    int error=0;
    ESP_LOGI(TAG, "Setting bind options");
    error=ssh_bind_options_set(sc->sc_sshbind, SSH_BIND_OPTIONS_LOG_VERBOSITY_STR, "1");
    if(error!=0) {
        ESP_LOGE(TAG, "ssh_bind_options_set SSH_BIND_OPTIONS_LOG_VERBOSITY_STR returned=%i",error);
		return SSH_ERROR;
	}
    error=ssh_bind_options_set(sc->sc_sshbind, SSH_BIND_OPTIONS_HOSTKEY, "/ssh/hostkey");
    if(error!=0) {
        ESP_LOGE(TAG, "ssh_bind_options_set SSH_BIND_OPTIONS_HOSTKEY returned=%i",error);
		return SSH_ERROR;
	}
    error=ssh_bind_options_set(sc->sc_sshbind, SSH_BIND_OPTIONS_BINDPORT_STR, "22");
    if(error!=0) {
        ESP_LOGE(TAG, "ssh_bind_options_set SSH_BIND_OPTIONS_BINDPORT_STR returned=%i",error);
		return SSH_ERROR;
	}
	error=ssh_bind_options_set(sc->sc_sshbind, SSH_BIND_OPTIONS_BINDADDR, "0.0.0.0");
    if(error!=0) {
        ESP_LOGE(TAG, "ssh_bind_options_set SSH_BIND_OPTIONS_BINDADDR returned=%i",error);
		return SSH_ERROR;
	}

    ESP_LOGI(TAG, "Setting callbacks");
	error=ssh_bind_set_callbacks(sc->sc_sshbind, &sc->sc_bind_cb, sc);
    if(error!=0) {
        ESP_LOGE(TAG, "ssh_bind_set_callbacks returned=%i",error);
        return SSH_ERROR;
    }
    ESP_LOGI(TAG, "Calling ssh_bind_listen()");
    error=ssh_bind_listen(sc->sc_sshbind);
	if(error!=0) {
        ESP_LOGE(TAG, "ssh_bind_listen returned=%i",error);
		ssh_bind_free(sc->sc_sshbind);
		return SSH_ERROR;
	}
    ESP_LOGI(TAG, "Calling ssh_bind_set_blocking()");
	ssh_bind_set_blocking(sc->sc_sshbind, 0);
    
    ESP_LOGI(TAG, "Calling ssh_event_add_poll()");
	error=ssh_event_add_poll(sc->sc_sshevent, ssh_bind_get_poll(sc->sc_sshbind));
    if(error!=0) {
        ESP_LOGE(TAG, "ssh_event_add_poll returned=%i",error);
        return SSH_ERROR;
    }
	return SSH_OK;
}

static void
terminate_server(struct server_ctx *sc)
{
	struct client_ctx *cc;

	ssh_event_remove_poll(sc->sc_sshevent, ssh_bind_get_poll(sc->sc_sshbind));
	close(ssh_bind_get_fd(sc->sc_sshbind));
	SLIST_FOREACH(cc, &sc->sc_client_head, cc_client_list) {
		ssh_silent_disconnect(cc->cc_session);
	}
	while (!SLIST_EMPTY(&sc->sc_client_head)) {
		(void) ssh_event_dopoll(sc->sc_sshevent, 100);
		dead_eater(sc);
	}
	ssh_bind_free(sc->sc_sshbind);
	free(sc);
}


int
sshd_main(const char *hardcoded_example_host_key)
{
	struct server_ctx *sc;
	sc = calloc(1, sizeof(struct server_ctx));
	if (!sc)
		return -1;
	sc->sc_host_key = hardcoded_example_host_key;
	sc->sc_begin_interactive_session = minicli_begin_interactive_session;
	sc->sc_auth_methods = SSH_AUTH_METHOD_PASSWORD;
   
	ESP_LOGI(TAG, "ssh_init()");
	if (ssh_init()!=SSH_OK) {
        ESP_LOGE(TAG, "ssh_init()!=SSH_OK");
		return SSH_ERROR;
	}
	ESP_LOGI(TAG, "ssh_event_new()");
	ssh_event event = ssh_event_new();
	if (!event){
        ESP_LOGE(TAG, "ssh_event_new()");
		return SSH_ERROR;
	}
    sc->sc_sshevent=event;
	ESP_LOGI(TAG, "create_new_server(sc)");
	if (create_new_server(sc, hardcoded_example_host_key) != SSH_OK){
		ESP_LOGE(TAG, "create_new_server(sc)");
		return SSH_ERROR;
	}
	ESP_LOGI(TAG, "Init complete. Start eternal loop!");
    bool time_to_die = false;
	while (!time_to_die) {
		int error = ssh_event_dopoll(sc->sc_sshevent, 1000);
		if (error == SSH_ERROR || error == SSH_AGAIN) {
            //ESP_LOGE(TAG, "Error while doing eternal loop=%i", error);
			/* check if any clients are dead and consume 'em */
			dead_eater(sc);
		}
	}
	terminate_server(sc);
	ssh_event_free(event);
	ssh_finalize();
	return SSH_OK;
}


static void
handle_char_from_local(struct interactive_session *is, char c)
{
	struct client_ctx *cc = (struct client_ctx *)((uintptr_t)is - offsetof(struct client_ctx, cc_is));
	ssh_channel_write(cc->cc_channel, &c, 1);
}