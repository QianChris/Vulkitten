#include <Vulkitten.h>

class Sandbox : public Vulkitten::Application {
public:
    Sandbox() {}
    ~Sandbox() {}
};

Vulkitten::Application* Vulkitten::CreateApplication() {
    return new Sandbox();
}
