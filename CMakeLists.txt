cmake_minimum_required(VERSION 3.8)
project(ros2_bag_manager)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(rosbag2_storage_default_plugins REQUIRED)


# 実行可能ファイルの作成
add_executable(extract_topic src/extract_topic.cpp)
ament_target_dependencies(extract_topic 
  rclcpp 
  rosbag2_cpp 
  rosbag2_storage_default_plugins
)

# インストールターゲットの指定
install(TARGETS
  extract_topic
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

ament_package()
