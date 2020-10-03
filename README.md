# asio_redis_client
an thread-safe, easy to use async redis client implemented in c++11.

The best c++ redis client!

# dependcy
asio

future extention(submodule)

# how to use

```
git clone https://github.com/topcpporg/asio_redis_client.git

git submodule update --init --recursive
```

# quick example

## async interface

```
//create redis client
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

  client->del("hello", [](RedisValue value) {
    std::cout << "del: " << value.inspect() << '\n';
  });

  client->publish("mychannel", "hello world", [](RedisValue value) {
    std::cout << "publish mychannel ok, number:" << value.inspect() << '\n';
  });
}
```

## sync interface

fun with future!
```
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
```

# error handling

```
//network error
auto client = std::make_shared<asio_redis_client>(ios);
client->enable_auto_reconnect(true);

//network error occurred
client->set_error_callback([](RedisValue value){
  std::cout<<value.toString()<<'\n';
});

//redis command error
client->auth("123456", [](RedisValue value) {
  if(value.isError()){
    std::cout<<"redis error:"<<value.toString()<<'\n';
    return;
  }

  std::cout << "auth: " << value.toString() << '\n';
});
```

# important
If you like it, please start it, let more people know it the best c++ redis client :)

# roadmap

1. pipeline
2. redis cluster