#include "pch.h"
#include "Camera.h"
#include "Adonis/Math/Math.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Adonis {

	namespace rendersystem {

		Camera::Camera(){
			AD_ON_EVENT_BIND(KeyPressed, Camera);
			AD_ON_EVENT_BIND(KeyReleased, Camera);
			//AD_ON_EVENT_BIND(MouseMovedEvent, Camera);
			AD_ON_EVENT_BIND(MouseScrolledEvent, Camera);
			AD_ON_EVENT_BIND(UpdateEvent, Camera);
			using namespace math::literals;
			m_fov = 90._degf;
		}

		void Camera::update() {
			
			m_front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
			m_front.y = sin(glm::radians(m_pitch));
			m_front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
			m_front = glm::normalize(m_front);

			m_right = glm::normalize(glm::cross(m_front, m_world_up));
			m_up = glm::normalize(glm::cross(m_right, m_front));
		}

		AD_EVENT_FUNC_DEF_HEAD(UpdateEvent, Camera) {

			float distance = m_velocity * event->deltatime();

			switch (m_movement_status) {
			case MovementDirection::Forward:
				m_pos += m_front * distance;
				break;
			case MovementDirection::Backward:
				m_pos -= m_front * distance;
				AD_CORE_INFO("Moving cam backwards");
				break;
			case MovementDirection::Left:
				m_pos -= m_right * distance;
				break;
			case MovementDirection::Right:
				m_pos += m_right * distance;
				break;
			case MovementDirection::Down:
				m_pos -= m_up * distance;
				break;
			case MovementDirection::Up:
				m_pos += m_up * distance;
				break;
			case MovementDirection::Stationary: //No need to move
			default: break;
			}

		}

		AD_EVENT_FUNC_DEF_HEAD(KeyPressed, Camera) {
			switch (event->code()) {
			case ADONIS_KEY_W:
				m_movement_status = MovementDirection::Forward;
				break;
			case ADONIS_KEY_S:
				m_movement_status = MovementDirection::Backward;
				break;
			case ADONIS_KEY_A:
				m_movement_status = MovementDirection::Left;
				break;
			case ADONIS_KEY_D:
				m_movement_status = MovementDirection::Right;
				break;
			case ADONIS_KEY_C:
				m_movement_status = MovementDirection::Down;
				break;
			case ADONIS_KEY_SPACE:
				m_movement_status = MovementDirection::Up;
				break;
			}
		}

		AD_EVENT_FUNC_DEF_HEAD(KeyReleased, Camera) {
			switch (event->code()) {
			default: 
				m_movement_status = MovementDirection::Stationary;
				break;
			}
		}

		AD_EVENT_FUNC_DEF_HEAD(MouseMovedEvent, Camera) {
			static double last_x = 0;
			static double last_y = 0;
			static bool first_mouse = true;

			if (first_mouse) {
				last_x = event->xpos();
				last_y = event->ypos();
				first_mouse = false;
			}

			double xoff = event->xpos() - last_x;
			double yoff = event->ypos() - last_y;

			last_x = event->xpos();
			last_y = event->ypos();

			m_yaw += xoff * m_sensitivity;
			m_pitch += yoff * m_sensitivity;

			if (m_constrain_pitch) {
				if (m_pitch > 89.0f) {
					m_pitch = 89.0f;
				}
				else if (m_pitch < -89.0f) {
					m_pitch = -89.0f;
				}
			}
		}

		AD_EVENT_FUNC_DEF_HEAD(MouseScrolledEvent, Camera) {

			if (m_enable_zoom) {
				m_fov += event->yoffset();
			}

		}

		glm::mat4 Camera::view()const {
			return glm::lookAt(m_pos, m_front, m_up);
			//return glm::mat4(1.0f);
		}

		glm::mat4 Camera::projection()const {
			return glm::perspective(glm::radians(m_fov), m_aspectratio, m_clip_space.x, m_clip_space.y);
		}

	}
}