//
// Created by qicosmos on 2020/9/28.
//
#include <iostream>
#include <gtest/gtest.h>
#include "asio_redis_client.h"

using namespace purecpp;

boost::asio::io_service ios;
boost::asio::io_service::work work(ios);
std::thread thd([]{
  ios.run();
});
static const std::string host_name = "11.166.214.161";
static const std::string password = "123456";

std::shared_ptr<asio_redis_client> create_client(){
  auto client = std::make_shared<asio_redis_client>(ios);
  client->enable_auto_reconnect(true);

  client->set_error_callback([](RedisValue value){
    std::cout<<value.toString()<<'\n';
  });

  bool r = client->connect(host_name, 6379);
  assert(r);
  return client;
}

TEST(RedisClient, connect_failed){
  auto client = std::make_shared<asio_redis_client>(ios);
  client->enable_auto_reconnect(true);

  unsigned short invalid_port = 0;
  bool r = client->connect(host_name, invalid_port);
  EXPECT_FALSE(r);

  size_t has_try_times = client->connect_with_trytimes(host_name, invalid_port, 2);
  EXPECT_EQ(has_try_times, size_t(2));
}

//make sure the redis server has been started!
TEST(RedisClient, connect){
  auto client = std::make_shared<asio_redis_client>(ios);
  client->enable_auto_reconnect(true);

  bool r = client->connect(host_name, 6379);
  if(!r){
    return;
  }

  auto client1 = std::make_shared<asio_redis_client>(ios);
  size_t has_try_times = client1->connect_with_trytimes(host_name, 6379, 2);
  EXPECT_EQ(has_try_times, size_t(0));
}

TEST(RedisClient, auth_get_set_del) {
  auto client = create_client();
  client->auth(password, [=](RedisValue value) {
    EXPECT_TRUE(value.isError()); //if no password, will return error
    std::cout << "auth: " << value.toString() << '\n';
    client->set("hello", "world", [=](RedisValue value) {
      EXPECT_FALSE(value.isError());
      std::cout << "set: " << value.toString() << '\n';
      client->get("hello", [=](RedisValue value) {
        EXPECT_FALSE(value.isError());
        std::cout << "get: " << value.toString() << '\n';
        client->del("hello", [=](RedisValue value) {
          EXPECT_FALSE(value.isError());
          std::cout << "del: " << value.inspect() << '\n';
        });
      });
    });
  });
}

TEST(RedisClient, ping){
  auto client = create_client();
  client->ping([](RedisValue value) {
    EXPECT_FALSE(value.isError());
  });

  auto future = client->ping();
  EXPECT_FALSE(future.Get().isError());
}

TEST(RedisClient, publish){
  auto client = create_client();
  client->publish("mychannel", "hello world", [](RedisValue value) {
    EXPECT_FALSE(value.isError());
    std::cout << "publish mychannel ok, number:" << value.inspect() << '\n';
  });
}

bool stop_publish = false;
void create_publisher() {
  std::thread pub_thd([] {
    auto publisher = create_client();

    for (int i = 0; i < 3000; i++) {
      if (stop_publish) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));

      publisher->publish("mychannel", "hello world", [](RedisValue value) {
        EXPECT_FALSE(value.isError());
        std::cout << "publish mychannel ok, number:" << value.inspect() << '\n';
      });
    }
  });
  pub_thd.detach();
}

TEST(RedisClient, pub_sub){
  create_publisher();

  auto client = create_client();
  client->subscribe("mychannel", [=](RedisValue value) {
    EXPECT_FALSE(value.isError());
    std::cout << "subscribe mychannel: " << value.toString() << '\n';
    client->unsubscribe("mychannel", [=](RedisValue value) {
      EXPECT_FALSE(value.isError());
      std::cout << "unsubscribe mychannel: " << value.toString() << '\n';
    });
  });
}

TEST(RedisClient, unsub){
  auto client = create_client();
  client->unsubscribe("mychannel", [=](RedisValue value) {
    EXPECT_FALSE(value.isError());
    std::cout << "unsubscribe mychannel: " << value.toString() << '\n';
  });

  client->punsubscribe("mychanne*", [=](RedisValue value) {
    EXPECT_FALSE(value.isError());
    std::cout << "punsubscribe mychannel: " << value.toString() << '\n';
    stop_publish = true;
  });

  auto unsub_future = client->unsubscribe("mychannel");
  auto punsub_future = client->punsubscribe("mychanne*");
  EXPECT_FALSE(unsub_future.Get().isError());
  EXPECT_FALSE(punsub_future.Get().isError());
}

TEST(RedisClient, pub_psub){
  auto client = create_client();
  client->psubscribe("mychanne*", [=](RedisValue value) {
    EXPECT_FALSE(value.isError());
    std::cout << "psubscribe mychannel: " << value.toString() << '\n';
    client->punsubscribe("mychanne*", [=](RedisValue value) {
      EXPECT_FALSE(value.isError());
      std::cout << "punsubscribe mychannel: " << value.toString() << '\n';
      stop_publish = true;
    });
  });
}



TEST(RedisClient, command){
  auto client = create_client();
  client->command("info", {"stats"}, [](RedisValue value) {
    EXPECT_FALSE(value.isError());
    std::cout << "info stats: " << value.toString() << '\n';
  });

  client->command("get", {"hello"}, [](RedisValue value) {
    EXPECT_FALSE(value.isError());
    std::cout << "get result: " << value.toString() << '\n';
  });

  auto future = client->command("info", {"stats"});
  EXPECT_FALSE(future.Get().isError());
}

TEST(RedisClient, future) {
  auto client = create_client();

  auto auth_future = client->auth(password);
  auto set_future = client->set("hello", "world");
  auto get_future = client->get("hello");
  auto del_future = client->del("hello");

  EXPECT_TRUE(auth_future.Get().isError());
  EXPECT_FALSE(set_future.Get().isError());
  EXPECT_FALSE(get_future.Get().isError());
  EXPECT_FALSE(del_future.Get().isError());
  client->close();
}

TEST(RedisClient, future_then){
  auto client = create_client();
  auto future = client->auth(password).Then([&](RedisValue value){
    EXPECT_TRUE(value.isError());
    std::cout<<"auth result:"<<value.toString()<<'\n';
    return client->set("hello", "world").Get();
  }).Then([&](RedisValue value){
    EXPECT_FALSE(value.isError());
    std::cout<<"set result:"<<value.toString()<<'\n';
    return client->get("hello").Get();
  }).Then([&](RedisValue value){
    EXPECT_FALSE(value.isError());
    std::cout<<"get result:"<<value.toString()<<'\n';
    return client->del("hello").Get();
  }).Then([](RedisValue value){
    EXPECT_FALSE(value.isError());
    std::cout<<value.toString();
    return "del result: ok";
  });

  std::cout<<"future then---------------\n";
  auto result = future.Get();
  std::cout<<result<<'\n';
}

std::thread finally_thread;

TEST(RedisClient, future_then_finally){
  finally_thread = std::thread([]{
    auto client = create_client();
    client->auth(password).Then([=](RedisValue value){
      EXPECT_TRUE(value.isError());
      std::cout<<"auth result:"<<value.toString()<<'\n';
      return client->set("hello", "world").Get();
    }).Then([=](RedisValue value){
      EXPECT_FALSE(value.isError());
      std::cout<<"set result:"<<value.toString()<<'\n';
      return client->get("hello").Get();
    }).Then([=](RedisValue value){
      EXPECT_FALSE(value.isError());
      std::cout<<"get result:"<<value.toString()<<'\n';
      return client->del("hello").Get();
    }).Then([](RedisValue value){
      EXPECT_FALSE(value.isError());
      std::cout<<value.toString();
      return "del result: ok";
    }).Finally([](std::string result){
      std::cout<<result<<'\n';
    });
  });
}

TEST(RedisClient, close_stop){
  finally_thread.join();
  auto client = create_client();
  EXPECT_TRUE(client->has_connected());
  client->close();
  EXPECT_FALSE(client->has_connected());
  ios.stop();//the last case stop the io_service to quit all the test case.
  thd.join();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  auto result =  RUN_ALL_TESTS();
  return result;
}