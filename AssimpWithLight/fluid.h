#pragma once
#include <glm/glm.hpp>

#include <vector>

#include "Shader.h"
#include "texture.h"

// ��ʾһ����������
struct VertexFluid {
	glm::vec3 position;
	glm::vec2 texCoords;
};

class Fluid {
public:

	// rowSize_: x�������, columnSize_: z�������, step: �������
	// refreshTime_: ˢ��ʱ����
	// t: ��ʽϵ��-ʱ��, c: ��ʽϵ��-����, mu: ��ʽϵ��-����ϵ��
	Fluid(int rowSize_, int columnSize_, float step, GLfloat refreshTime_, float t, float c, float mu):
		rowSize(rowSize_), columnSize(columnSize_), refreshTime(refreshTime_),
		currentRender(false), accumulateTime(0.0f)
	{
		// ���������ʽϵ��
		float f1 = c * c * t * t / (step * step);
		float f2 = 1.0f / (mu * t + 2);
		k1 = (4.0f - 8.0f * f1) * f2;
		k2 = (mu * t - 2) * f2;
		k3 = 2.0f * f1 * f2;

		// ��������
		texture = TextureHelper::load2DTexture("water.bmp");
		
		int count = rowSize * columnSize;  // ��������
		// ���붥������
		vertData1.reserve(count);
		vertData2.reserve(count);
		float texStepX = 1.0f / rowSize;
		float textStepY = 1.0f / columnSize;
		for (int i = 0; i < columnSize; i++) {
			float z = step * i;
			for (int j = 0; j < rowSize; j++) {
				float x = step * j;
				float y = 0.0f;
				// �߽���0.0f ����������ʼ��0.0f��1.0f
				if (i != 0 && j != 0 && i != columnSize - 1 && j != columnSize - 1)
					y = (0 == rand() % 2) ? 1.0f : 0.0f;
				float textCoorX = j * texStepX;
				float textCoorY = i * textStepY;

				VertexFluid v;
				v.position.x = x;
				v.position.y = y;
				v.position.z = z;
				v.texCoords.x = textCoorX;
				v.texCoords.y = textCoorY;
				vertData1.push_back(v);
				vertData2.push_back(v);
			}
		}
		// ���������ο�������Ϣ
		indices.reserve(2 * ((rowSize - 1)*(columnSize - 1)));
		// ������
		for (int i = 0; i < rowSize - 1; i++) {
			for (int j = 0; j < columnSize - 1; j++) {
				int index = i * rowSize + j;
				indices.push_back(index);
				indices.push_back(index + 1);
				indices.push_back(index + rowSize);
			}
		}
		// ������
		for (int i = 1; i < rowSize; i++) {
			for (int j = 1; j < columnSize; j++) {
				int index = i * rowSize + j;
				indices.push_back(index);
				indices.push_back(index - 1);
				indices.push_back(index - rowSize);
			}
		}

		this->setupFluid();
	}
	~Fluid() {
		this->final();
	}

	// �趨ˮ��ˢ�·�Χ  ��Χ����Ϊ�����±�д��  ������
	bool setRefreshRange(int startX, int endX, int startZ, int endZ) {
		startX = startX < 1 ? 1 : startX;
		endX = endX > rowSize - 1 ? rowSize - 1 : endX;
		startZ = startZ < 1 ? 1 : startZ;
		endZ = endZ > columnSize - 1 ? columnSize - 1 : endZ;
		if (startX >= rowSize || endX < 0
			|| startZ >= columnSize || endZ < 0) {
			return false;
		}
		refreshRangeStartColumn = startX;
		refreshRangeEndColumn = endX;
		refreshRangeStartRow = startZ;
		refreshRangeEndRow = endZ;
		return true;
	}

	void animation(GLfloat deltaTime) {
		accumulateTime += deltaTime;
		if (accumulateTime > refreshTime) {
			this->refresh();
			accumulateTime = 0;
		}
	}
	// ǿ��ˢ����״
	void refresh(void) {
		// ����������淽��
		currentRender = !currentRender;
		std::vector<VertexFluid> &vd1 = currentRender ? vertData1 : vertData2;
		std::vector<VertexFluid> &vd2 = currentRender ? vertData2 : vertData1;
		for (int i = refreshRangeStartColumn; i <= refreshRangeEndColumn; i++) {
			for (int j = refreshRangeStartRow; j <= refreshRangeEndRow; j++) {
				int index = i * rowSize + j;
				float temp = vertData1[index].position.y;
				vd2[index].position.y =
					k1 * vd1[index].position.y +
					k2 * vd2[index].position.y +
					k3 * (vd1[index + 1].position.y + vd1[index - 1].position.y +
						vd1[index + rowSize].position.y + vd1[index - rowSize].position.y);
			}
		}
		//for (int i = 1; i < columnSize - 1; i++) {
		//	for (int j = 1; j < rowSize - 1; j++) {
		//		int index = i * rowSize + j;
		//		float temp = vertData1[index].position.y;
		//		vd2[index].position.y =
		//			k1 * vd1[index].position.y +
		//			k2 * vd2[index].position.y +
		//			k3 * (vd1[index + 1].position.y + vd1[index - 1].position.y +
		//				vd1[index + rowSize].position.y + vd1[index - rowSize].position.y);
		//	}
		//}
		glBindVertexArray(this->VAOId);
		glBindBuffer(GL_ARRAY_BUFFER, this->VBOId);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexFluid) * vd2.size(),
			&vd2[0], GL_STREAM_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// ����
	void draw(const Shader& shader) const {
		shader.use();
		glBindVertexArray(this->VAOId);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(shader.programId, "textureImage"), 0);

		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		glUseProgram(0);
	}

private:
	// ����VAO, VBO, EBO�Ȼ�����
	void setupFluid(void)
	{
		glGenVertexArrays(1, &this->VAOId);
		glGenBuffers(1, &this->VBOId);
		glGenBuffers(1, &this->EBOId);

		glBindVertexArray(this->VAOId);
		glBindBuffer(GL_ARRAY_BUFFER, this->VBOId);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexFluid) * this->vertData1.size(),
			&this->vertData1[0], GL_STREAM_DRAW);
		// ����λ������
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			sizeof(VertexFluid), (GLvoid*)0);
		glEnableVertexAttribArray(0);
		// ������������
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
			sizeof(VertexFluid), (GLvoid*)(3 * sizeof(GL_FLOAT)));
		glEnableVertexAttribArray(1);
		// ��������
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBOId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * this->indices.size(),
			&this->indices[0], GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void final(void) const {
		glDeleteVertexArrays(1, &this->VAOId);
		glDeleteBuffers(1, &this->VBOId);
		glDeleteBuffers(1, &this->EBOId);
	}

private:
	int rowSize;    // x������
	int columnSize;    // z������
	int refreshRangeStartColumn;
	int refreshRangeEndColumn;
	int refreshRangeStartRow;
	int refreshRangeEndRow;

	// �������ʽ��Ҫ��¼��һ�ε��λ�� ���ʹ�����鶥�����ݽ��渳ֵ
	bool currentRender;    // ��¼��ǰʹ�õĶ��㻺��
	std::vector<VertexFluid> vertData1;    // ���㻺��1
	std::vector<VertexFluid> vertData2;    // ���㻺��2
	
	std::vector<GLuint> indices;    // ����

	float k1, k2, k3;    // ����ʽϵ��

	GLfloat refreshTime;
	GLfloat accumulateTime;

	GLuint VAOId, VBOId, EBOId;
	GLuint texture;    // ����
};