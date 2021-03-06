#include "pch.h"
#include "WindowGLFW.h"

#include "Adonis/Eventsystem/EventManager.h"
#include "Adonis/Eventsystem/Events/Events.h"
#include <stb_image.h>


namespace Adonis {

	static bool s_glfw_initialized = false;
	static bool prev_vsync;

	static void GLFWErrorCallback(int error, const char* desc) {
		AD_CORE_ERROR("GLFW reported error: {0} with description: {1}", error, desc);
	}

	std::unique_ptr<Window> Window::create(const uint16_t& width, const uint16_t& height, const std::string& title, const bool& vsync, WindowMode mode) {
		return std::make_unique<WindowGLFW>(width, height, title, vsync, mode);
	}

	WindowGLFW::WindowGLFW(const uint16_t& width, const uint16_t& height, const std::string& title, const bool& vsync, WindowMode mode) :
		m_width(width),
		m_height(height),
		m_title(title),
		m_mode(mode),
		m_vsync(vsync){				
		init();
		prev_vsync = vsync;
	}

	WindowGLFW::~WindowGLFW() {
	}

	void WindowGLFW::init() {
		
		if (!s_glfw_initialized) {
			int status = glfwInit();
			AD_CORE_ASSERT(status, "Failed to initialize GLFW");
			glfwSetErrorCallback(GLFWErrorCallback);
			s_glfw_initialized = true;
		}

#ifdef ADONIS_USE_OPENGL
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif ADONIS_USE_OPENGL

		switch (m_mode) {
		case WindowMode::Windowed:
			m_window = std::unique_ptr<GLFWwindow, void(*)(GLFWwindow *)>(glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr), WindowGLFW::shutdown);
			break;
		case WindowMode::Borderless:
		{
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* vid_mode = glfwGetVideoMode(monitor);
			glfwWindowHint(GLFW_RED_BITS, vid_mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, vid_mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, vid_mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, vid_mode->refreshRate);

			m_window = std::unique_ptr<GLFWwindow, void(*)(GLFWwindow *)>(glfwCreateWindow(vid_mode->width, vid_mode->height, m_title.c_str(), monitor, nullptr), glfwDestroyWindow);
		}
			break;
		case WindowMode::Fullscreen:
			m_window = std::unique_ptr<GLFWwindow, void(*)(GLFWwindow *)>(glfwCreateWindow(m_width, m_height, m_title.c_str(), glfwGetPrimaryMonitor(), nullptr), glfwDestroyWindow);
			break;
		default:break;
		}

		//Set member variables
		glfwGetFramebufferSize(m_window.get(), &m_framebuffer_width, &m_framebuffer_height);

		glfwMakeContextCurrent(m_window.get());
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		AD_CORE_ASSERT(status, "Failed to initialize GLAD");
		glfwSwapInterval(m_vsync);
		
		glfwSetKeyCallback(m_window.get(), [] (GLFWwindow* window, int key, int scancode, int action, int mods){
			switch (action) {
			case GLFW_PRESS:
				EventManager::queueEvent<KeyPressed>(key, mods);
				break;
			case GLFW_RELEASE:
				EventManager::queueEvent<KeyReleased>(key, mods);				
				break;
			case GLFW_REPEAT://TODO
				break;
			default: break;
			}
		});

		glfwSetMouseButtonCallback(m_window.get(), [](GLFWwindow*, int button, int action, int mods) {
			switch (action) {
			case GLFW_PRESS:
				EventManager::queueEvent<MouseButtonPressed>(button, mods);
				break;
			case GLFW_RELEASE:
				EventManager::queueEvent<MouseButtonReleased>(button, mods);
				break;
			case GLFW_REPEAT://TODO
				break;
			default: break;
			}
		});

		glfwSetCursorPosCallback(m_window.get(), [](GLFWwindow*, double x, double y) {
			EventManager::queueEvent<MouseMovedEvent>(x, y);
		});

		glfwSetScrollCallback(m_window.get(), [](GLFWwindow*, double xoffset, double yoffset) {
			EventManager::queueEvent<MouseScrolledEvent>(xoffset, yoffset);
		});

		glfwSetWindowSizeCallback(m_window.get(), [](GLFWwindow*, int width, int height) {
			if (width == 0 || height == 0) return;
			EventManager::queueEvent<WindowResizeEvent>(width, height);
		});

		glfwSetWindowCloseCallback(m_window.get(), [](GLFWwindow*) {
			EventManager::queueEvent<WindowCloseEvent>();
		});

		glfwSetCharCallback(m_window.get(), [](GLFWwindow*, unsigned int c) {
			EventManager::queueEvent<CharTyped>(c);
		});

		AD_ON_EVENT_BIND(UpdateEvent,		WindowGLFW);
		AD_ON_EVENT_BIND(PreRenderEvent,	WindowGLFW);
		AD_ON_EVENT_BIND(RenderEvent,		WindowGLFW);
		AD_ON_EVENT_BIND(PostRenderEvent,	WindowGLFW);
		AD_ON_EVENT_BIND(WindowResizeEvent, WindowGLFW);

		//TEMPORARY
		int width, height, n;
		unsigned char * data = stbi_load("WindowIcon.png", &width, &height, &n, 0);
		if (data == nullptr) {
			AD_CORE_ERROR("Failed to load window icon");
		}
		else {
			GLFWimage* image = new GLFWimage;
			image->height = height;
			image->width = width;
			image->pixels = data;
			glfwSetWindowIcon(m_window.get(), 1, image);
			delete image;
			stbi_image_free(data);
		}
		//END

	}

	void WindowGLFW::toggle_fullscreen() {
		auto monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		if (m_mode != WindowMode::Fullscreen) {
			glfwSetWindowMonitor(m_window.get(), monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			m_mode = WindowMode::Fullscreen;
		}
		else {
			glfwSetWindowMonitor(m_window.get(), nullptr, 0, 30, m_width, m_height, mode->refreshRate);
			m_mode = WindowMode::Windowed;
		}
		glfwSwapInterval(m_vsync);
	}

	void WindowGLFW::on_UpdateEvent(const event_ptr_t<UpdateEvent>& ev) {
		glfwGetWindowSize(m_window.get(), &m_width, &m_height);
		glfwGetFramebufferSize(m_window.get(), &m_framebuffer_width, &m_framebuffer_height);
		glfwGetCursorPos(m_window.get(), &m_mouse_pos.x, &m_mouse_pos.y);
		if (prev_vsync != m_vsync) {
			update_vsync();
		}
		prev_vsync = m_vsync;
	}

	void WindowGLFW::shutdown(GLFWwindow* window) {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void WindowGLFW::on_WindowCloseEvent(const event_ptr_t<WindowCloseEvent>& event) {
	}

	void WindowGLFW::on_WindowResizeEvent(const event_ptr_t<WindowResizeEvent>& event) {
		m_width = event->width();
		m_height = event->height();
	}

	void WindowGLFW::on_PreRenderEvent(const event_ptr_t<PreRenderEvent>& event) {
		glfwPollEvents();
	}

	void WindowGLFW::on_RenderEvent(const event_ptr_t<RenderEvent>& event) {
		
	}

	void WindowGLFW::on_PostRenderEvent(const event_ptr_t<PostRenderEvent>& event) {
		glfwSwapBuffers(m_window.get());
	}

	bool WindowGLFW::has_focus()const {
		return static_cast<bool>(glfwGetWindowAttrib(m_window.get(), GLFW_FOCUSED));
	}

	void WindowGLFW::set_mouse_pos(double x, double y) {
		glfwSetCursorPos(m_window.get(), x, y);
	}

	bool& WindowGLFW::vsync() {
		return m_vsync;
	}

	void WindowGLFW::update_vsync() {
		glfwSwapInterval(m_vsync);
	}

	glm::ivec2 WindowGLFW::pos() {
		int x, y;
		glfwGetWindowPos(m_window.get(), &x, &y);
		return { x, y };
	}

	void WindowGLFW::toggle_cursor() {
		m_show_cursor = !m_show_cursor;
		if (m_show_cursor) {
			this->enable_cursor();
		}
		else {
			this->disable_cursor();
		}
	}

	void WindowGLFW::enable_cursor() {
		glfwSetInputMode(m_window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	void WindowGLFW::disable_cursor() {
		glfwSetInputMode(m_window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}