set(priv_requires "led_strip")

idf_component_register(SRC_DIRS "."
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES ${priv_requires})
