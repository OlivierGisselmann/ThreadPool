project(ThreadPoolTests)

# Get Catch2
Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)

# Create executable from sources
add_executable(${PROJECT_NAME} threadpool_test.cpp)

# Link libraries to executable
target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain ThreadPool)