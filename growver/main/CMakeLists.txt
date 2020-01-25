set(COMPONENT_SRCS "main.c" "commandline.c" "growver_mdns.c" "ota-http.c" "file_server.c" "growver_rest.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_EMBED_TXTFILES WebFiles/index.html WebFiles/ota-page.html WebFiles/favicon.ico  WebFiles/upload_script.html)
register_component()
