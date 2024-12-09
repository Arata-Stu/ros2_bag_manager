#include <rosbag2_cpp/reader.hpp>
#include <rosbag2_cpp/writer.hpp>
#include <rosbag2_storage/storage_options.hpp>
#include <rosbag2_storage_default_plugins/sqlite/sqlite_storage.hpp>
#include <rclcpp/rclcpp.hpp>
#include <memory>
#include <string>

class BagFilterNode : public rclcpp::Node {
public:
    BagFilterNode() : Node("extract_topic") {
        // パラメータの宣言
        this->declare_parameter<std::string>("input_bag_path", "/path/to/input_bag");
        this->declare_parameter<std::string>("output_bag_path", "/path/to/output_bag");
        this->declare_parameter<std::string>("target_topic", "/desired_topic");

        // パラメータの取得
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
        storage_options_in.storage_id = "sqlite3"; // 入力ファイルのストレージ形式
        reader->open(storage_options_in);

        auto writer = std::make_shared<rosbag2_cpp::Writer>();
        rosbag2_storage::StorageOptions storage_options_out;
        storage_options_out.uri = output_bag_path;
        storage_options_out.storage_id = "sqlite3"; // 出力ファイルのストレージ形式
        writer->open(storage_options_out);

        // リーダーのメッセージを読み込み、対象トピックのメッセージをフィルタリング
        bool topic_created = false;

        while (reader->has_next()) {
            auto bag_message = reader->read_next();

            // 対象トピックの場合
            if (bag_message->topic_name == target_topic) {
                // トピックがまだ作成されていない場合に作成
                if (!topic_created) {
                    // トピックの型を取得
                    auto topic_type = reader->get_metadata().topics_with_message_count;
                    auto it = std::find_if(topic_type.begin(), topic_type.end(),
                        [&target_topic](const auto& topic_info) {
                            return topic_info.topic_metadata.name == target_topic;
                        });

                    if (it != topic_type.end()) {
                        writer->create_topic(it->topic_metadata);
                        topic_created = true;
                    } else {
                        RCLCPP_ERROR(this->get_logger(), "Target topic type not found: %s", target_topic.c_str());
                        break;
                    }
                }

                // メッセージを書き込む
                writer->write(bag_message);
            }
        }

        RCLCPP_INFO(this->get_logger(), "Filtered bag file created at: %s", output_bag_path.c_str());

        // 処理が終わったのでノードを停止
        rclcpp::shutdown();
    }
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);

    // ノードを初期化して即座に処理を実行
    auto node = std::make_shared<BagFilterNode>();

    // ノードをスピンしてから終了
    rclcpp::spin(node);

    return 0;
}
