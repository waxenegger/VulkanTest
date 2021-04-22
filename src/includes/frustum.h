#ifndef SRC_INCLUDES_FRUSTUM_H_
#define SRC_INCLUDES_FRUSTUM_H_

static constexpr float INF = std::numeric_limits<float>::infinity();
static constexpr float NEG_INF = -1 * std::numeric_limits<float>::infinity();
static constexpr glm::vec3 INFINITY_VECTOR3 = glm::vec3(INF);
static constexpr glm::vec3 NEGATIVE_INFINITY_VECTOR3 = glm::vec3(NEG_INF);
static constexpr glm::mat4 IDENTITY_MATRIX4 = glm::mat4(1.0f);

struct BoundingBox final {
    public:
        glm::vec3 min = glm::vec3(INF);
        glm::vec3 max = glm::vec3(NEG_INF);
};

class Frustum final
{
    private:
        enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };
        std::array<glm::vec4, 6> planes;
        BoundingBox bbox;
        
    public:

        void update(glm::mat4 matrix)
        {
            planes[LEFT].x = matrix[0].w + matrix[0].x;
            planes[LEFT].y = matrix[1].w + matrix[1].x;
            planes[LEFT].z = matrix[2].w + matrix[2].x;
            planes[LEFT].w = matrix[3].w + matrix[3].x;

            planes[RIGHT].x = matrix[0].w - matrix[0].x;
            planes[RIGHT].y = matrix[1].w - matrix[1].x;
            planes[RIGHT].z = matrix[2].w - matrix[2].x;
            planes[RIGHT].w = matrix[3].w - matrix[3].x;

            planes[TOP].x = matrix[0].w - matrix[0].y;
            planes[TOP].y = matrix[1].w - matrix[1].y;
            planes[TOP].z = matrix[2].w - matrix[2].y;
            planes[TOP].w = matrix[3].w - matrix[3].y;

            planes[BOTTOM].x = matrix[0].w + matrix[0].y;
            planes[BOTTOM].y = matrix[1].w + matrix[1].y;
            planes[BOTTOM].z = matrix[2].w + matrix[2].y;
            planes[BOTTOM].w = matrix[3].w + matrix[3].y;

            planes[BACK].x = matrix[0].w + matrix[0].z;
            planes[BACK].y = matrix[1].w + matrix[1].z;
            planes[BACK].z = matrix[2].w + matrix[2].z;
            planes[BACK].w = matrix[3].w + matrix[3].z;

            planes[FRONT].x = matrix[0].w - matrix[0].z;
            planes[FRONT].y = matrix[1].w - matrix[1].z;
            planes[FRONT].z = matrix[2].w - matrix[2].z;
            planes[FRONT].w = matrix[3].w - matrix[3].z;

            BoundingBox bboxTmp;
            
            for (uint32_t  i = 0; i < planes.size(); i++)
            {
                float length = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
                planes[i] /= length;
                if (planes[i].x < bbox.min.x) bbox.min.x = planes[i].x;
                if (planes[i].x > bbox.max.x) bbox.max.x = planes[i].x;
                if (planes[i].y < bbox.min.y) bbox.min.y = planes[i].y;
                if (planes[i].y > bbox.max.y) bbox.max.y = planes[i].y;
                if (planes[i].z < bbox.min.z) bbox.min.z = planes[i].z;
                if (planes[i].z > bbox.max.z) bbox.max.z = planes[i].z;                
            }
            
            this->bbox = bboxTmp;
        }

        bool checkSphere(glm::vec3 pos, float radius)
        {
            for (uint32_t i = 0; i < planes.size(); i++)
            {
                if ((planes[i].x * pos.x) + (planes[i].y * pos.y) + (planes[i].z * pos.z) + planes[i].w <= -radius)
                {
                    return false;
                }
            }
            return true;
        }
        
        BoundingBox getBoundingBox() {
            return this->bbox;
        }
};

#endif
