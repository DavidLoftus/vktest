#ifdef _MSC_VER
#	pragma once
#endif
#ifndef CAMERA_H
#define CAMERA_H


class Camera
{
public:
	glm::mat4 getProjectionMatrix() const;
	glm::mat4 getViewMatrix() const;

	void input();

private:
	glm::vec3 m_position;
	glm::vec3 m_direction;
};

#endif