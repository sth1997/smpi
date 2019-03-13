ADD_EXECUTABLE(hello hello.cpp)
ADD_EXECUTABLE(test_barrier test_barrier.cpp)
TARGET_LINK_LIBRARIES(hello smpi)
TARGET_LINK_LIBRARIES(test_barrier smpi)