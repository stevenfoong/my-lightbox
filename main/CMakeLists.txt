file(GLOB_RECURSE IMAGE_SOURCES images/*.c)


idf_component_register(SRCS "main.c" 
                            "lvgl_demo_ui.c" 
                            "wifi.c" 
                            "webserver.c"
                            "storage.c"
                            "nvs.c"
                            ${IMAGE_SOURCES}
                            INCLUDE_DIRS "."
                            EMBED_FILES ../wifi-manager/style.css ../wifi-manager/code.js ../wifi-manager/index.html)