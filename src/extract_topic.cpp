#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <rosbag2_storage_default_plugins/sqlite/sqlite_storage.hpp>
#include <rclcpp/rclcpp.hpp>
#include <memory>
#include <string>
#include <atomic>
#include <thread>

class BagFilterNode : public rclcpp::Node {
public:
    BagFilterNode() : Node("extract_topic"), processing_done_(false) {
        // 非同期処理を開始
        processing_thread_ = std::thread(&BagFilterNode::process_bag, this);
    }

    ~BagFilterNode() {
        if (processing_thread_.joinable()) {
            processing_thread_.join();
        }
    }

    bool is_processing_done() const {
        return processing_done_.load();
    }

private:
    std::atomic<bool> processing_done_;
    std::thread processing_thread_;

    void process_bag() {
        // パラメータの宣言と取得
        this->declare_parameter<std::string>("input_bag_path", "/path/to/input_bag");
        this->declare_parameter<std::string>("output_bag_path", "/path/to/output_bag");
        this->declare_parameter<std::string>("target_topic", "/desired_topic");

        std::string input_bag_path = this->get_parameter("input_bag_path").as_string();
        std::string output_bag_path = this->get_parameter("output_bag_path").as_string();
        std::string target_topic = this->get_parameter("target_topic").as_string();

        RCLCPP_INFO(this->get_logger(), "Input bag: %s", input_bag_path.c_str());
        RCLCPP_INFO(this->get_logger(), "Output bag: %s", output_bag_path.c_str());
        RCLCPP_INFO(this->get_logger(), "Target topic: %s", target_topic.c_str());

        // ROS2 bagのリーダーとライターの初期化
        auto reader = std::make_shared<rosbag2_cpp::Reader>();
        rosbag2_storage::StorageOptions storage_options_in;
        storage_options_in.uri = input_bag_path;
        storage_options_in.storage_id = "sqlite3";
        reader->open(storage_options_in);

        auto writer = std::make_shared<rosbag2_cpp::Writer>();
        rosbag2_storage::StorageOptions storage_options_out;
        storage_options_out.uri = output_bag_path;
        storage_options_out.storage_id = "sqlite3";
        writer->open(storage_options_out);

        // メッセージのフィルタリングと書き込み
        bool topic_created = false;

        while (reader->has_next()) {
            auto bag_message = reader->read_next();

            if (bag_message->topic_name == target_topic) {
                if (!topic_created) {
                    auto topic_type = reader->get_metadata().topics_with_message_count;
                    auto it = std::find_if(topic_type.begin(), topic_type.end(),
                        [&target_topic](const auto& topic_info) {
                            return topic_info.topic_metadata.name == target_topic;
                        });

                    if (it != topic_type.end()) {
                        writer->create_topic(it->topic_metadata);
                        topic_created = true;
                        RCLCPP_INFO(this->get_logger(), "Target topic created: %s", target_topic.c_str());
                    } else {
                        RCLCPP_ERROR(this->get_logger(), "Target topic type not found: %s", target_topic.c_str());
                        processing_done_ = true; // 終了フラグを設定
                        return;
                    }
                }
                writer->write(bag_message);
            }
        }

        RCLCPP_INFO(this->get_logger(), "Filtered bag file created at: %s", output_bag_path.c_str());
        processing_done_ = true; // 処理終了を通知
    }
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);

    auto node = std::make_shared<BagFilterNode>();

    // スピンを特定の条件で終了する
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);

    while (!node->is_processing_done()) {
        executor.spin_some();
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 小休止
    }

    rclcpp::shutdown();
    return 0;
}
