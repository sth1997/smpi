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
allreduce.cpp
)

ADD_LIBRARY(smpi SHARED ${SOURCE_FILES})
INSTALL(TARGETS smpi LIBRARY DESTINATION lib)

ADD_EXECUTABLE(run_smartnic run_smartnic.cpp)
TARGET_LINK_LIBRARIES(run_smartnic smpi)
INSTALL(TARGETS run_smartnic RUNTIME DESTINATION bin)