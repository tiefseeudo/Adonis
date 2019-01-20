#include "Adonis.h"
#include <iostream>
#include <memory>

class Sandbox : public Adonis::Application {
public:
	Sandbox() {
		layers().push_layer<Adonis::ImGuiLayer>(Adonis::ImGuiLayer::Style::Extasy);
	};
	~Sandbox() {};
};


Adonis::Application* Adonis::createApplication() {
	return new Sandbox();
}
	