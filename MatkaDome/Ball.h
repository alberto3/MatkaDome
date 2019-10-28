#ifndef BALL_H
#define BALL_H

class Ball {
public:
    Ball() {}

    Ball(uint32_t i_Duration, uint8_t i_Color) {
        m_Color = i_Color;
        m_Duration = i_Duration;
    }

    virtual ~Ball() {}

    uint8_t GetColor() const {
        return m_Color;
    }

    void SetColor(uint8_t i_Color) {
        m_Color = i_Color;
    }

    uint32_t GetDuration() const {
        return m_Duration;
    }

    void SetDuration(uint32_t i_Duration) {
        m_Duration = i_Duration;
    }

protected:
    uint8_t m_Color;
    uint32_t m_Duration;
};


#endif
