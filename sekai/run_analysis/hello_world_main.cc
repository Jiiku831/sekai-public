#include <iostream>
#include <string_view>

std::string HelloWorld(std::string_view user) {
  if (user.empty()) {
    return "Hello world!";
  }
  return "Hello world, " + std::string(user) + "!";
}

int main(int argc, const char** argv) {
  std::string response;
  if (argc < 2) {
    response = HelloWorld("");
  } else {
    response = HelloWorld(argv[1]);
  }
  std::cout << response << std::endl;
  return 0;
}
