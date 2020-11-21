#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;
struct Inputs {
    float4 src;
    float4 dst;
};
struct Outputs {
    float4 sk_FragColor [[color(0)]];
};


fragment Outputs fragmentMain(Inputs _in [[stage_in]], bool _frontFacing [[front_facing]], float4 _fragCoord [[position]]) {
    Outputs _outputStruct;
    thread Outputs* _out = &_outputStruct;
    float4 _0_blend_saturation;
    {
        float _1_alpha = _in.dst.w * _in.src.w;
        float3 _2_sda = _in.src.xyz * _in.dst.w;
        float3 _3_dsa = _in.dst.xyz * _in.src.w;
        float3 _4_blend_set_color_saturation;
        {
            float _5_17_blend_color_saturation;
            {
                _5_17_blend_color_saturation = max(max(_2_sda.x, _2_sda.y), _2_sda.z) - min(min(_2_sda.x, _2_sda.y), _2_sda.z);
            }
            float _6_sat = _5_17_blend_color_saturation;

            if (_3_dsa.x <= _3_dsa.y) {
                if (_3_dsa.y <= _3_dsa.z) {
                    float3 _7_18_blend_set_color_saturation_helper;
                    {
                        _7_18_blend_set_color_saturation_helper = _3_dsa.x < _3_dsa.z ? float3(0.0, (_6_sat * (_3_dsa.y - _3_dsa.x)) / (_3_dsa.z - _3_dsa.x), _6_sat) : float3(0.0);
                    }
                    _4_blend_set_color_saturation = _7_18_blend_set_color_saturation_helper;

                } else if (_3_dsa.x <= _3_dsa.z) {
                    float3 _8_19_blend_set_color_saturation_helper;
                    {
                        _8_19_blend_set_color_saturation_helper = _3_dsa.x < _3_dsa.y ? float3(0.0, (_6_sat * (_3_dsa.z - _3_dsa.x)) / (_3_dsa.y - _3_dsa.x), _6_sat) : float3(0.0);
                    }
                    _4_blend_set_color_saturation = _8_19_blend_set_color_saturation_helper.xzy;

                } else {
                    float3 _9_20_blend_set_color_saturation_helper;
                    {
                        _9_20_blend_set_color_saturation_helper = _3_dsa.z < _3_dsa.y ? float3(0.0, (_6_sat * (_3_dsa.x - _3_dsa.z)) / (_3_dsa.y - _3_dsa.z), _6_sat) : float3(0.0);
                    }
                    _4_blend_set_color_saturation = _9_20_blend_set_color_saturation_helper.yzx;

                }
            } else if (_3_dsa.x <= _3_dsa.z) {
                float3 _10_21_blend_set_color_saturation_helper;
                {
                    _10_21_blend_set_color_saturation_helper = _3_dsa.y < _3_dsa.z ? float3(0.0, (_6_sat * (_3_dsa.x - _3_dsa.y)) / (_3_dsa.z - _3_dsa.y), _6_sat) : float3(0.0);
                }
                _4_blend_set_color_saturation = _10_21_blend_set_color_saturation_helper.yxz;

            } else if (_3_dsa.y <= _3_dsa.z) {
                float3 _11_22_blend_set_color_saturation_helper;
                {
                    _11_22_blend_set_color_saturation_helper = _3_dsa.y < _3_dsa.x ? float3(0.0, (_6_sat * (_3_dsa.z - _3_dsa.y)) / (_3_dsa.x - _3_dsa.y), _6_sat) : float3(0.0);
                }
                _4_blend_set_color_saturation = _11_22_blend_set_color_saturation_helper.zxy;

            } else {
                float3 _12_23_blend_set_color_saturation_helper;
                {
                    _12_23_blend_set_color_saturation_helper = _3_dsa.z < _3_dsa.x ? float3(0.0, (_6_sat * (_3_dsa.y - _3_dsa.z)) / (_3_dsa.x - _3_dsa.z), _6_sat) : float3(0.0);
                }
                _4_blend_set_color_saturation = _12_23_blend_set_color_saturation_helper.zyx;

            }
        }
        float3 _13_blend_set_color_luminance;
        {
            float _14_15_blend_color_luminance;
            {
                _14_15_blend_color_luminance = dot(float3(0.30000001192092896, 0.5899999737739563, 0.10999999940395355), _3_dsa);
            }
            float _15_lum = _14_15_blend_color_luminance;

            float _16_16_blend_color_luminance;
            {
                _16_16_blend_color_luminance = dot(float3(0.30000001192092896, 0.5899999737739563, 0.10999999940395355), _4_blend_set_color_saturation);
            }
            float3 _17_result = (_15_lum - _16_16_blend_color_luminance) + _4_blend_set_color_saturation;

            float _18_minComp = min(min(_17_result.x, _17_result.y), _17_result.z);
            float _19_maxComp = max(max(_17_result.x, _17_result.y), _17_result.z);
            if (_18_minComp < 0.0 && _15_lum != _18_minComp) {
                _17_result = _15_lum + ((_17_result - _15_lum) * _15_lum) / (_15_lum - _18_minComp);
            }
            _13_blend_set_color_luminance = _19_maxComp > _1_alpha && _19_maxComp != _15_lum ? _15_lum + ((_17_result - _15_lum) * (_1_alpha - _15_lum)) / (_19_maxComp - _15_lum) : _17_result;
        }
        _0_blend_saturation = float4((((_13_blend_set_color_luminance + _in.dst.xyz) - _3_dsa) + _in.src.xyz) - _2_sda, (_in.src.w + _in.dst.w) - _1_alpha);


    }
    _out->sk_FragColor = _0_blend_saturation;

    return *_out;
}
