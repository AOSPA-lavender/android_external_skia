struct FSIn {
  @builtin(front_facing) sk_Clockwise: bool,
  @builtin(position) sk_FragCoord: vec4<f32>,
};
struct FSOut {
  @location(0) sk_FragColor: vec4<f32>,
};
struct _GlobalUniforms {
  testInputs: vec4<f32>,
  colorGreen: vec4<f32>,
  colorRed: vec4<f32>,
};
@binding(0) @group(0) var<uniform> _globalUniforms: _GlobalUniforms;
fn main(_skParam0: vec2<f32>) -> vec4<f32> {
  let coords = _skParam0;
  {
    const expected: vec4<f32> = vec4<f32>(0.75, 0.0, 0.75, 0.25);
    let _skTemp0 = fract(_globalUniforms.testInputs.x);
    let _skTemp1 = fract(_globalUniforms.testInputs.xy);
    let _skTemp2 = fract(_globalUniforms.testInputs.xyz);
    let _skTemp3 = fract(_globalUniforms.testInputs);
    return select(_globalUniforms.colorRed, _globalUniforms.colorGreen, vec4<bool>((((_skTemp0 == 0.75) && all(_skTemp1 == vec2<f32>(0.75, 0.0))) && all(_skTemp2 == vec3<f32>(0.75, 0.0, 0.75))) && all(_skTemp3 == expected)));
  }
}
@fragment fn fragmentMain(_stageIn: FSIn) -> FSOut {
  var _stageOut: FSOut;
  _stageOut.sk_FragColor = main(_stageIn.sk_FragCoord.xy);
  return _stageOut;
}
