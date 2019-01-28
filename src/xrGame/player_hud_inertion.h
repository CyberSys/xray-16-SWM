#pragma once

/**************************/
/***** Инерция у худа *****/ //--#SM+#--
/**************************/

// clang-format off
#define PITCH_OFFSET_R       0.017f   // Насколько сильно ствол смещается вбок (влево) при вертикальных поворотах камеры
#define PITCH_OFFSET_N       0.012f   // Насколько сильно ствол поднимается\опускается при вертикальных поворотах камеры
#define PITCH_OFFSET_D       0.02f    // Насколько сильно ствол приближается\отдаляется при вертикальных поворотах камеры
#define PITCH_LOW_LIMIT     -PI       // Минимальное значение pitch при использовании совместно с PITCH_OFFSET_N
#define TENDTO_SPEED         1.0f     // Модификатор силы инерции (больше - чувствительней)
#define TENDTO_SPEED_AIM     1.0f     // (Для прицеливания)
#define TENDTO_SPEED_RET     5.0f     // Модификатор силы отката инерции (больше - быстрее)
#define TENDTO_SPEED_RET_AIM 5.0f     // (Для прицеливания)
#define INERT_MIN_ANGLE      0.0f     // Минимальная сила наклона, необходимая для старта инерции
#define INERT_MIN_ANGLE_AIM  3.5f     // (Для прицеливания)

// Пределы смещения при инерции (лево / право / верх / низ)
#define ORIGIN_OFFSET        0.04f,  0.04f,  0.04f, 0.02f 
#define ORIGIN_OFFSET_AIM    0.015f, 0.015f, 0.01f, 0.005f   

// Outdated - old inertion
#define TENDTO_SPEED_OLD       5.f      // Скорость нормализации положения ствола
#define TENDTO_SPEED_AIM_OLD   8.f      // (Для прицеливания)
#define ORIGIN_OFFSET_OLD     -0.05f    // Фактор влияния инерции на положение ствола (чем меньше, тем маштабней инерция)
#define ORIGIN_OFFSET_AIM_OLD -0.03f    // (Для прицеливания)

// clang-format on
