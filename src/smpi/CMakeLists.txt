SET(SOURCE_FILES
init.cpp
finalize.cpp
proc.cpp
common.cpp
comm_size.cpp
comm_rank.cpp
send.cpp
recv.cpp
barrier.cpp
)

ADD_LIBRARY(smpi SHARED ${SOURCE_FILES})
INSTALL(TARGETS smpi LIBRARY DESTINATION lib)