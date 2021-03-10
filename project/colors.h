typedef struct Color
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
} Color;
inline Color color(u8 r,u8 g,u8 b,u8 a)
{
    Color result;
    result.r=r;
    result.g=g;
    result.b=b;
    result.a=a;
    return result;
}
const Color BASE_COLORS[]=
{
    color(40, 40, 40, 255),
    color(45, 153, 153, 255),
    color(153, 153, 45, 255),
    color(153, 45, 153, 255),
    color(45, 153, 0x51, 255),
    color(153, 45, 45, 255),
    color(45, 99, 153, 255),
    color(153, 99, 45, 255)
};
const Color LIGHT_COLORS[]=
{
    color(40, 40, 40, 255),
    color(68, 229, 229, 255),
    color(229, 229, 68, 255),
    color(229, 68, 229, 255),
    color(68, 229, 122, 255),
    color(229, 68, 68, 255),
    color(68, 149, 229, 255),
    color(229, 149, 68, 255)
};

const Color DARK_COLORS[]=
{
    color(40, 40, 40, 255),
    color(30, 102, 102, 255),
    color(102, 102, 30, 255),
    color(102, 30, 102, 255),
    color(30, 102, 54, 255),
    color(102, 30, 30, 255),
    color(30, 66, 102, 255),
    color(102, 66, 30, 255)
};
