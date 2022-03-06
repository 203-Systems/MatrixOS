#include "MatrixBoot.h"

void MatrixBoot::Setup()
{

}

bool MatrixBoot::Idle(bool ready)
{  
    if(timer.Tick(80))
    {
        MatrixOS::LED::Fill(0);
        uint8_t step = counter % 12;
        // MatrixOS::LED::SetColor(Point(7, step), Color(0x404040));
        if(step <= 3)
        {
            Point line_origin = origin + Point(-1,-1) + Point(0, step);
            for(uint8_t i = 0; i < step + 1; i++)
            {
                MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, Color(0x404040));
            }
        }
        else if(step <= 6)
        {
            Point line_origin = origin + Point(0,2) + Point(step - 4, 0);
            for(uint8_t i = 0; i < 3 - (step - 4); i++)
            {
                MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, Color(0x404040));
            }
        }
        MatrixOS::LED::Update();
        counter ++;
        if(step > 6)
            return false;
    }
    return true;
}


void MatrixBoot::Boot()
{ 
    switch(boot_phase)
    {
        
        case 0:
            MatrixOS::LED::Fill(0);
            counter = 0;
            boot_phase ++;
        case 1:
            BootPhase1();
            break;
        case 2:
            BootPhase2();
            break;
    }
}

void MatrixBoot::BootPhase1()
{
    if(timer.Tick(80))
    {
        if(counter <= 3)
        {
            Point line_origin = origin + Point(-1,-1) + Point(0, counter);
            for(uint8_t i = 0; i < counter + 1; i++)
            {
                MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, Color(0x808080));
            }
        }
        else if(counter <= 6)
        {
            Point line_origin = origin + Point(0,2) + Point(counter - 4, 0);
            for(uint8_t i = 0; i < 3 - (counter - 4); i++)
            {
                MatrixOS::LED::SetColor(line_origin + Point(1, -1) * i, Color(0x808080));
            }
        }
        MatrixOS::LED::Update();
        if(counter == 8) //Hold on delay
        {
            boot_phase ++;
            counter = 0;
            return;
        }
        counter ++;
    }
}

Color MatrixBoot::BootPhase2Color(int16_t time, uint8_t hue)
{
    if(time < 0)
    {
        return Color(0);
    }
    if(time < 200)
    {
        return Color(0x808080);
    }
    else if(time < 600 )
    {
        if(hue == 127)
            return Color(0x008080);
        else if(hue == 212)
            return Color(0x800080);
    }
    else
    {
        return Color(0);
    }

    return Color(0);
}

void MatrixBoot::BootPhase2QuadSetColor(uint8_t x_offset, uint8_t y_offset, Color color1, Color color2)
{
    Point point_q1 = origin + Point(1,1) + Point(x_offset, y_offset);
    MatrixOS::LED::SetColor(point_q1, color2);

    Point point_q2 = origin + Point(0,1) + Point(-x_offset, y_offset);
    MatrixOS::LED::SetColor(point_q2, color1);

    Point point_q3 = origin + Point(0,0) + Point(-x_offset, -y_offset);
    MatrixOS::LED::SetColor(point_q3, color2);

    Point point_q4 = origin + Point(1,0) + Point(x_offset, -y_offset);
    MatrixOS::LED::SetColor(point_q4, color1);
}

#define max(a,b) ((a)>(b)?(a):(b))
void MatrixBoot::BootPhase2()
{
    const uint8_t hue[2] = {127, 212};
    const uint16_t start_offset = 100;

    if(boot_phase_2_start_time == 0)
        boot_phase_2_start_time = MatrixOS::SYS::Millis();
    
    uint32_t delta_time = MatrixOS::SYS::Millis() - boot_phase_2_start_time;
    uint8_t quad_size = max(Device::x_size, Device::y_size) / 2;
    if(delta_time > (quad_size - 2) * 100 + 600 + 100)
    {
        Exit();
    }
    
    for(uint8_t r = 0; r < quad_size; r++) //radius
    {
        
        uint16_t local_deltatime = delta_time - (r - 2) * start_offset;
        Color color1 = BootPhase2Color(local_deltatime, hue[0]);
        Color color2 = BootPhase2Color(local_deltatime, hue[1]);
        BootPhase2QuadSetColor(r, r, color1, color2);
        if(r > 0)
        {
            uint16_t local_deltatime_half = local_deltatime + start_offset / 2;
            Color half_color1 = BootPhase2Color(local_deltatime_half, hue[0]);
            Color half_color2 = BootPhase2Color(local_deltatime_half, hue[1]);
            BootPhase2QuadSetColor(r - 1, r, half_color1, half_color2);
            BootPhase2QuadSetColor(r, r - 1, half_color1, half_color2);
        }
    }
    MatrixOS::LED::Update();
}

void MatrixBoot::End()
{
    MatrixOS::LED::Fill(0);
    MatrixOS::LED::Update();
}