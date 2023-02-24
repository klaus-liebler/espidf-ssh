# Embed the server root certificate into the final binary
#
# (If this was a component, we would set COMPONENT_EMBED_TXTFILES here.)
idf_component_register(SRCS "main.c" "minicli.c" "sshd_task.c" "sshd.c"
                    INCLUDE_DIRS ".")
