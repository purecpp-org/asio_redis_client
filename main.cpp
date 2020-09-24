//
// Created by qicosmos on 2020/9/9.
//
#include <iostream>
#include <set>
#include "asio_redis_client.h"

using namespace purecpp;
void test_redis_client(){
  boost::asio::io_service ios;
  boost::asio::io_service::work work(ios);
  std::thread thd([&ios]{
    ios.run();
  });

  std::thread pub_thd([&ios]{
    auto publisher = std::make_shared<asio_redis_client>(ios);
    bool r = publisher->connect("11.166.214.161", 6379);
    if(r){
      std::cout<<"redis connected\n";
    }else{
      std::cout<<"redis not connect\n";
    }

    for(int i=0; i<300; i++){
      std::this_thread::sleep_for(std::chrono::seconds(3));
      publisher->publish("mychannel", "hello world", [](RedisValue value){
        std::cout<<"publish mychannel ok, number:"<<value.inspect()<<'\n';
      });
    }
  });

  auto client = std::make_shared<asio_redis_client>(ios);
  bool r = client->connect("11.166.214.161", 6379);
  if(r){
    std::cout<<"redis connected\n";
  }else{
    std::cout<<"redis not connect\n";
  }

  client->set_error_callback([](RedisValue value){
    std::cout<<value.inspect()<<'\n';
  });

  for (int i = 0; i < 100; ++i) {
    client->set("hello", "world", [](RedisValue value){
      std::cout<<value.toString()<<'\n';
    });

    client->get("hello", [](RedisValue value){
      std::cout<<value.toString()<<'\n';
    });
  }

  client->subscribe("mychannel", [&client](RedisValue value){
    std::cout<<"subscribe mychannel: "<<value.toString()<<'\n';
    client->unsubscribe("mychannel", [](RedisValue value){
      std::cout<<"unsubscribe mychannel: "<<value.toString()<<'\n';
    });
  });

  client->psubscribe("my_subscribe_key:*", [](RedisValue value){
    std::cout<<"psubscribe: "<<value.toString()<<'\n';
  });

  pub_thd.join();
  thd.join();
}

int main(){
  test_redis_client();
  return 0;
}