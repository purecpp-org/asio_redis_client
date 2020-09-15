//
// Created by qicosmos on 2020/9/9.
//
#include <iostream>
#include "asio_redis_client.h"
using namespace purecpp;
void test_connect(){
  boost::asio::io_service ios;
  boost::asio::io_service::work work(ios);
  std::thread thd([&ios]{
    ios.run();
  });
  auto client = std::make_shared<asio_redis_client>(ios);
  bool r = client->connect("175.24.34.5", 6379);
//  bool r = client->connect_with_trytimes("127.0.0.1", 6379, 0);
//  r = client->connect_with_trytimes("127.0.0.1", 6379, 2);
  if(r){
    std::cout<<"redis connected\n";
  }else{
    std::cout<<"redis not connect\n";
  }

  client->set("hello", "world");

//  client->get("unique-redis-key-example");

//"*3\r\n$3\r\nset\r\n$24\r\nnunique-redis-key-example\r\n$18\r\nunique-redis-value\r\n"
//  client->command("*3\r\n$3\r\nSET\r\n$24\r\nunique-redis-key-example\r\n$18\r\nunique-redis-value\r\n");

  thd.join();
}

int main(){
  test_connect();
  return 0;
}