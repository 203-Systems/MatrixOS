set(component_srcs "blemidi.c")
idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES bt nvs_flash console)