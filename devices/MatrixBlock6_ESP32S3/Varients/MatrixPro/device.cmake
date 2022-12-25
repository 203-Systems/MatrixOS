# Apply board specific content here

idf_build_get_property(idf_target IDF_TARGET)

message(STATUS "Apply ${DEVICE}(${idf_target}) specific options")