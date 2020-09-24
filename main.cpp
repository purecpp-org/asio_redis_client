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

  auto client = std::make_shared<asio_redis_client>(ios);
  bool r = client->connect("127.0.0.1", 6379);
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

  client->subscribe("my_subscribe_key:d3668dac747fe2be7b52db24479709e3d42b3ab4", [](RedisValue value){
    std::cout<<"subscribe: "<<value.toString()<<'\n';
  });

  client->psubscribe("my_subscribe_key:*", [](RedisValue value){
    std::cout<<"psubscribe: "<<value.toString()<<'\n';
  });

  thd.join();
}

int main(){
  test_redis_client();
  return 0;
}