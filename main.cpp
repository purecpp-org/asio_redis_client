//
// Created by qicosmos on 2020/9/9.
//
#include <iostream>
#include <set>
#include <regex>
#include "asio_redis_client.h"

using namespace purecpp;
boost::asio::io_service ios;
boost::asio::io_service::work work(ios);
std::thread thd([]{
  ios.run();
});
static const std::string host_name = "127.0.0.1";

std::shared_ptr<asio_redis_client> create_client(){
  auto client = std::make_shared<asio_redis_client>(ios);
  client->enable_auto_reconnect(true);

  bool r = client->connect(host_name, 6379);
  assert(r);
  return client;
}

void async_connect(){
  auto client = std::make_shared<asio_redis_client>(ios);
  client->enable_auto_reconnect(true);
  client->async_connect(host_name, 6379, [client](RedisValue value){
    if(value.isOk()){
      client->set("hello", "world", [](RedisValue value) {
        std::cout << "set: " << value.toString() << '\n';
      });

      client->get("hello", [](RedisValue value) {
        std::cout << "get: " << value.toString() << '\n';
      });
    }
  });

  while(true){
    std::string str;
    std::cin >> str;
  }

}

void test_retry(){
  auto client = create_client();

  client->command("get", {"hello"}, 2/*retry_times*/,[](RedisValue value) {
    std::cout << "get result: " << value.toString() << '\n';
  });

  client->command("set", {"hello", "world"}, 2/*retry_times*/,[](RedisValue value) {
    std::cout << "set: " << value.toString() << '\n';
  });


  std::string str;
  std::cin >> str;

}

void get_set() {
  auto client = create_client();

  client->auth("123456", [](RedisValue value) {
    if(value.isError()){
      std::cout<<"redis error:"<<value.toString()<<'\n';
    }

    std::cout << "auth: " << value.toString() << '\n';
  });

  client->set("hello", "world", [](RedisValue value) {
    std::cout << "set: " << value.toString() << '\n';
  });

  client->get("hello", [](RedisValue value) {
    std::cout << "get: " << value.toString() << '\n';
  });

  client->command("info", {"stats"}, [](RedisValue value) {
    std::cout << "info stats: " << value.toString() << '\n';
  });

  client->command("get", {"hello"}, [](RedisValue value) {
    std::cout << "get result: " << value.toString() << '\n';
  });

  client->command( "get", {"hello"}, 2,[](RedisValue value) {
      std::cout << "get: " << value.toString() << '\n';
  });

  client->del("hello", [](RedisValue value) {
    std::cout << "del: " << value.inspect() << '\n';
  });

  std::string str;
  std::cin >> str;
}

bool stop = false;
void create_publisher() {
  std::thread pub_thd([] {
    auto publisher = create_client();

    for (int i = 0; i < 3000; i++) {
      if (stop) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));

      publisher->publish("mychannel", "hello world", [](RedisValue value) {
        std::cout << "publish mychannel ok, number:" << value.inspect() << '\n';
      });

    }
  });
  pub_thd.detach();
}

void pub_sub() {
  create_publisher();

  auto client = create_client();
  client->subscribe("mychannel", [=](RedisValue value) {
    std::cout << "subscribe mychannel: " << value.toString() << '\n';
//    client->unsubscribe("mychannel", [](RedisValue value) {
//      std::cout << "unsubscribe mychannel: " << value.toString() << '\n';
//    });
  });

  std::string str;
  std::cin >> str;

  client->unsubscribe("mychannel", [](RedisValue value) {
    std::cout << "unsubscribe mychannel: " << value.toString() << '\n';
  });

  stop = true;
}

void reconnect() {
  auto client = std::make_shared<asio_redis_client>(ios);
  client->enable_auto_reconnect(true);
  client->connect(host_name, 6379);
  std::string str;
  std::cin >> str;
  client->ping([](RedisValue value) {
    std::cout << "ping: " << value.toString() << '\n';
  });
}

void reconnect_withtimes() {
  auto client = std::make_shared<asio_redis_client>(ios);
  client->enable_auto_reconnect(true);
  client->connect_with_trytimes(host_name, 6379, 30);
  std::string str;
  std::cin >> str;
  client->ping([](RedisValue value) {
    std::cout << "ping: " << value.toString() << '\n';
  });
}

void callback_hell() {
  auto client = create_client();
  client->auth("123456", [=](RedisValue value) {
    std::cout << "auth: " << value.toString() << '\n';
    client->set("hello", "world", [=](RedisValue value) {
      std::cout << "set: " << value.toString() << '\n';
      client->get("hello", [=](RedisValue value) {
        std::cout << "get: " << value.toString() << '\n';
        client->del("hello", [=](RedisValue value) {
          std::cout << "del: " << value.inspect() << '\n';
        });
      });
    });
  });
}

#ifdef USE_FUTURE
void future(){
  auto client = create_client();

  auto auth_future = client->auth("123456");
  auto set_future = client->set("hello", "world");
  auto get_future = client->get("hello");
  auto del_future = client->del("hello");

  std::cout<<"future------------\n";
  std::cout<<"auth result:"<<auth_future.Get().toString()<<"\n";
  std::cout<<"set result:"<<set_future.Get().toString()<<'\n';
  std::cout<<"get result:"<<get_future.Get().toString()<<'\n';
  std::cout<<"del result: ok"<<del_future.Get().toString()<<'\n';
  client->close();
}

void future_then(){
  auto client = create_client();
  auto future = client->auth("123456").Then([&](RedisValue value){
    std::cout<<"auth result:"<<value.toString()<<'\n';
    return client->set("hello", "world").Get();
  }).Then([&](RedisValue value){
    std::cout<<"set result:"<<value.toString()<<'\n';
    return client->get("hello").Get();
  }).Then([&](RedisValue value){
    std::cout<<"get result:"<<value.toString()<<'\n';
    return client->del("hello").Get();
  }).Then([](RedisValue value){
    std::cout<<value.toString();
    return "del result: ok";
  });

  std::cout<<"future then---------------\n";
  auto result = future.Get();
  std::cout<<result<<'\n';
}

void future_then_finally(){
  auto client = create_client();
  client->auth("123456").Then([=](RedisValue value){
    std::cout<<"auth result:"<<value.toString()<<'\n';
    return client->set("hello", "world").Get();
  }).Then([=](RedisValue value){
    std::cout<<"set result:"<<value.toString()<<'\n';
    return client->get("hello").Get();
  }).Then([=](RedisValue value){
    std::cout<<"get result:"<<value.toString()<<'\n';
    return client->del("hello").Get();
  }).Then([](RedisValue value){
    std::cout<<value.toString();
    return "del result: ok";
  }).Finally([](std::string result){
    std::cout<<result<<'\n';
  });
  std::cout<<"future then finally---------------\n";
}
#endif

void test_future() {
#ifdef USE_FUTURE
  future();
  future_then();
  future_then_finally();
#endif
}

int main() {
//  async_connect();
//  reconnect();
//  reconnect_withtimes();
  test_retry();
  get_set();
  pub_sub();
  callback_hell();

  test_future();

  std::string str;
  std::cin >> str;

  ios.stop();
  thd.join();

  return 0;
}