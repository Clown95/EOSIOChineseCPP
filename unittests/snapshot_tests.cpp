
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 *@文件
 *@eos/license中定义的版权
 **/


#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <eosio/testing/tester.hpp>

#include <eosio/chain/snapshot.hpp>

#include <snapshot_test/snapshot_test.wast.hpp>
#include <snapshot_test/snapshot_test.abi.hpp>

#include <sstream>

using namespace eosio;
using namespace testing;
using namespace chain;

class snapshotted_tester : public base_tester {
public:
   snapshotted_tester(controller::config config, const snapshot_reader_ptr& snapshot, int ordinal) {
      FC_ASSERT(config.blocks_dir.filename().generic_string() != "."
         && config.state_dir.filename().generic_string() != ".", "invalid path names in controller::config");

      controller::config copied_config = config;
      copied_config.blocks_dir =
              config.blocks_dir.parent_path() / std::to_string(ordinal).append(config.blocks_dir.filename().generic_string());
      copied_config.state_dir =
              config.state_dir.parent_path() / std::to_string(ordinal).append(config.state_dir.filename().generic_string());

      init(copied_config, snapshot);
   }

   snapshotted_tester(controller::config config, const snapshot_reader_ptr& snapshot, int ordinal, int copy_block_log_from_ordinal) {
      FC_ASSERT(config.blocks_dir.filename().generic_string() != "."
         && config.state_dir.filename().generic_string() != ".", "invalid path names in controller::config");

      controller::config copied_config = config;
      copied_config.blocks_dir =
              config.blocks_dir.parent_path() / std::to_string(ordinal).append(config.blocks_dir.filename().generic_string());
      copied_config.state_dir =
              config.state_dir.parent_path() / std::to_string(ordinal).append(config.state_dir.filename().generic_string());

//创建所需块日志的副本，并可反转
      auto block_log_path = config.blocks_dir.parent_path() / std::to_string(copy_block_log_from_ordinal).append(config.blocks_dir.filename().generic_string());
      fc::create_directories(copied_config.blocks_dir);
      fc::copy(block_log_path / "blocks.log", copied_config.blocks_dir / "blocks.log");
      fc::copy(block_log_path / config::reversible_blocks_dir_name, copied_config.blocks_dir / config::reversible_blocks_dir_name );

      init(copied_config, snapshot);
   }
   /*Ned_block_ptr product_block（fc:：microseconds skip_time=fc:：millises（config:：block_interval_ms），uint32_t skip_flag=0/*skip_missed_block_penalty*/）override_
      返回_product_块（skip_time，false，skip_flag）；
   }

   有符号的\u块\u ptr产生\u空\u块（fc:：microseconds skip \u time=fc:：millises（config:：block \u interval \u ms），uint32 \u t skip \u flag=0/*skip \u missed \u块\u刑罚*/ )override {

      control->abort_block();
      return _produce_block(skip_time, true, skip_flag);
   }

   signed_block_ptr finish_block()override {
      return _finish_block();
   }

   bool validate() { return true; }
};

struct variant_snapshot_suite {
   using writer_t = variant_snapshot_writer;
   using reader_t = variant_snapshot_reader;
   using write_storage_t = fc::mutable_variant_object;
   using snapshot_t = fc::variant;

   struct writer : public writer_t {
      writer( const std::shared_ptr<write_storage_t>& storage )
      :writer_t(*storage)
      ,storage(storage)
      {

      }

      std::shared_ptr<write_storage_t> storage;
   };

   struct reader : public reader_t {
      explicit reader(const snapshot_t& storage)
      :reader_t(storage)
      {}
   };


   static auto get_writer() {
      return std::make_shared<writer>(std::make_shared<write_storage_t>());
   }

   static auto finalize(const std::shared_ptr<writer>& w) {
      w->finalize();
      return snapshot_t(*w->storage);
   }

   static auto get_reader( const snapshot_t& buffer) {
      return std::make_shared<reader>(buffer);
   }

};

struct buffered_snapshot_suite {
   using writer_t = ostream_snapshot_writer;
   using reader_t = istream_snapshot_reader;
   using write_storage_t = std::ostringstream;
   using snapshot_t = std::string;
   using read_storage_t = std::istringstream;

   struct writer : public writer_t {
      writer( const std::shared_ptr<write_storage_t>& storage )
      :writer_t(*storage)
      ,storage(storage)
      {

      }

      std::shared_ptr<write_storage_t> storage;
   };

   struct reader : public reader_t {
      explicit reader(const std::shared_ptr<read_storage_t>& storage)
      :reader_t(*storage)
      ,storage(storage)
      {}

      std::shared_ptr<read_storage_t> storage;
   };


   static auto get_writer() {
      return std::make_shared<writer>(std::make_shared<write_storage_t>());
   }

   static auto finalize(const std::shared_ptr<writer>& w) {
      w->finalize();
      return w->storage->str();
   }

   static auto get_reader( const snapshot_t& buffer) {
      return std::make_shared<reader>(std::make_shared<read_storage_t>(buffer));
   }

};

BOOST_AUTO_TEST_SUITE(snapshot_tests)

using snapshot_suites = boost::mpl::list<variant_snapshot_suite, buffered_snapshot_suite>;

BOOST_AUTO_TEST_CASE_TEMPLATE(test_exhaustive_snapshot, SNAPSHOT_SUITE, snapshot_suites)
{
   tester chain;

   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), snapshot_test_wast);
   chain.set_abi(N(snapshot), snapshot_test_abi);
   chain.produce_blocks(1);
   chain.control->abort_block();

   static const int generation_count = 8;
   std::list<snapshotted_tester> sub_testers;

   for (int generation = 0; generation < generation_count; generation++) {
//新建快照子级
      auto writer = SNAPSHOT_SUITE::get_writer();
      chain.control->write_snapshot(writer);
      auto snapshot = SNAPSHOT_SUITE::finalize(writer);

//在此快照中创建新子级
      sub_testers.emplace_back(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), generation);

//增加测试合同
      chain.push_action(N(snapshot), N(increment), N(snapshot), mutable_variant_object()
         ( "value", 1 )
      );

//生产块
      auto new_block = chain.produce_block();

//从检测仪中撤消自动挂起
      chain.control->abort_block();

      auto integrity_value = chain.control->calculate_integrity_hash();

//将该块推送到所有子测试人员，并在完成后验证数据库的完整性。
      for (auto& other: sub_testers) {
         other.push_block(new_block);
         BOOST_REQUIRE_EQUAL(integrity_value.str(), other.control->calculate_integrity_hash().str());
      }
   }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(test_replay_over_snapshot, SNAPSHOT_SUITE, snapshot_suites)
{
   tester chain;

   chain.create_account(N(snapshot));
   chain.produce_blocks(1);
   chain.set_code(N(snapshot), snapshot_test_wast);
   chain.set_abi(N(snapshot), snapshot_test_abi);
   chain.produce_blocks(1);
   chain.control->abort_block();

   static const int pre_snapshot_block_count = 12;
   static const int post_snapshot_block_count = 12;

   for (int itr = 0; itr < pre_snapshot_block_count; itr++) {
//增加合同
      chain.push_action(N(snapshot), N(increment), N(snapshot), mutable_variant_object()
         ( "value", 1 )
      );

//生产块
      chain.produce_block();
   }

   chain.control->abort_block();
   auto expected_pre_integrity_hash = chain.control->calculate_integrity_hash();

//新建快照子级
   auto writer = SNAPSHOT_SUITE::get_writer();
   chain.control->write_snapshot(writer);
   auto snapshot = SNAPSHOT_SUITE::finalize(writer);

//在此快照中创建新子级
   snapshotted_tester snap_chain(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), 1);
   BOOST_REQUIRE_EQUAL(expected_pre_integrity_hash.str(), snap_chain.control->calculate_integrity_hash().str());

//推送更多块以建立块日志
   for (int itr = 0; itr < post_snapshot_block_count; itr++) {
//增加合同
      chain.push_action(N(snapshot), N(increment), N(snapshot), mutable_variant_object()
         ( "value", 1 )
      );

//生产和推块
      snap_chain.push_block(chain.produce_block());
   }

//验证末尾的哈希
   chain.control->abort_block();
   auto expected_post_integrity_hash = chain.control->calculate_integrity_hash();
   BOOST_REQUIRE_EQUAL(expected_post_integrity_hash.str(), snap_chain.control->calculate_integrity_hash().str());

//从快照子级、从快照重播块日志
   snapshotted_tester replay_chain(chain.get_config(), SNAPSHOT_SUITE::get_reader(snapshot), 2, 1);
   BOOST_REQUIRE_EQUAL(expected_post_integrity_hash.str(), snap_chain.control->calculate_integrity_hash().str());
}

BOOST_AUTO_TEST_SUITE_END()