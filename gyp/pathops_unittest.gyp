# GYP file to build unit tests.
{
  'includes': [
    'apptype_console.gypi',
  ],
  'targets': [
    {
      'target_name': 'pathops_unittest',
      'type': 'executable',
      'suppress_wildcard': '1',
      'include_dirs' : [
        '../include/pathops',
        '../src/core',
        '../src/effects',
        '../src/lazy',
        '../src/pathops',
        '../src/pdf',
        '../src/pipe/utils',
        '../src/utils',
        '../tools/',
      ],
      'sources': [
        '../include/pathops/SkPathOps.h',
        '../src/pathops/SkAddIntersections.cpp',
        '../src/pathops/SkDCubicIntersection.cpp',
        '../src/pathops/SkDCubicLineIntersection.cpp',
        '../src/pathops/SkDCubicToQuads.cpp',
        '../src/pathops/SkDLineIntersection.cpp',
        '../src/pathops/SkDQuadImplicit.cpp',
        '../src/pathops/SkDQuadIntersection.cpp',
        '../src/pathops/SkDQuadLineIntersection.cpp',
        '../src/pathops/SkIntersections.cpp',
        '../src/pathops/SkOpAngle.cpp',
        '../src/pathops/SkOpContour.cpp',
        '../src/pathops/SkOpEdgeBuilder.cpp',
        '../src/pathops/SkOpSegment.cpp',
        '../src/pathops/SkPathOpsBounds.cpp',
        '../src/pathops/SkPathOpsCommon.cpp',
        '../src/pathops/SkPathOpsCubic.cpp',
        '../src/pathops/SkPathOpsDebug.cpp',
        '../src/pathops/SkPathOpsLine.cpp',
        '../src/pathops/SkPathOpsOp.cpp',
        '../src/pathops/SkPathOpsPoint.cpp',
        '../src/pathops/SkPathOpsQuad.cpp',
        '../src/pathops/SkPathOpsRect.cpp',
        '../src/pathops/SkPathOpsSimplify.cpp',
        '../src/pathops/SkPathOpsTriangle.cpp',
        '../src/pathops/SkPathOpsTypes.cpp',
        '../src/pathops/SkPathWriter.cpp',
        '../src/pathops/SkQuarticRoot.cpp',
        '../src/pathops/SkReduceOrder.cpp',
        '../src/pathops/SkAddIntersections.h',
        '../src/pathops/SkDQuadImplicit.h',
        '../src/pathops/SkIntersectionHelper.h',
        '../src/pathops/SkIntersections.h',
        '../src/pathops/SkLineParameters.h',
        '../src/pathops/SkOpAngle.h',
        '../src/pathops/SkOpContour.h',
        '../src/pathops/SkOpEdgeBuilder.h',
        '../src/pathops/SkOpSegment.h',
        '../src/pathops/SkOpSpan.h',
        '../src/pathops/SkPathOpsBounds.h',
        '../src/pathops/SkPathOpsCommon.h',
        '../src/pathops/SkPathOpsCubic.h',
        '../src/pathops/SkPathOpsCurve.h',
        '../src/pathops/SkPathOpsDebug.h',
        '../src/pathops/SkPathOpsLine.h',
        '../src/pathops/SkPathOpsPoint.h',
        '../src/pathops/SkPathOpsQuad.h',
        '../src/pathops/SkPathOpsRect.h',
        '../src/pathops/SkPathOpsSpan.h',
        '../src/pathops/SkPathOpsTriangle.h',
        '../src/pathops/SkPathOpsTypes.h',
        '../src/pathops/SkPathWriter.h',
        '../src/pathops/SkQuarticRoot.h',
        '../src/pathops/SkReduceOrder.h',
        '../src/pathops/TSearch.h',
        '../tests/PathOpsBoundsTest.cpp',
        '../tests/PathOpsCubicIntersectionTest.cpp',
        '../tests/PathOpsCubicIntersectionTestData.cpp',
        '../tests/PathOpsCubicLineIntersectionTest.cpp',
        '../tests/PathOpsCubicReduceOrderTest.cpp',
        '../tests/PathOpsCubicToQuadsTest.cpp',
        '../tests/PathOpsDCubicTest.cpp',
        '../tests/PathOpsDLineTest.cpp',
        '../tests/PathOpsDPointTest.cpp',
        '../tests/PathOpsDQuadTest.cpp',
        '../tests/PathOpsDRectTest.cpp',
        '../tests/PathOpsDTriangleTest.cpp',
        '../tests/PathOpsDVectorTest.cpp',
        '../tests/PathOpsExtendedTest.cpp',
        '../tests/PathOpsLineIntersectionTest.cpp',
        '../tests/PathOpsLineParametetersTest.cpp',
        '../tests/PathOpsOpCubicThreadedTest.cpp',
        '../tests/PathOpsOpRectThreadedTest.cpp',
        '../tests/PathOpsOpTest.cpp',
        '../tests/PathOpsQuadIntersectionTest.cpp',
        '../tests/PathOpsQuadIntersectionTestData.cpp',
        '../tests/PathOpsQuadLineIntersectionTest.cpp',
        '../tests/PathOpsQuadLineIntersectionThreadedTest.cpp',
        '../tests/PathOpsQuadParameterizationTest.cpp',
        '../tests/PathOpsQuadReduceOrderTest.cpp',
        '../tests/PathOpsSimplifyDegenerateThreadedTest.cpp',
        '../tests/PathOpsSimplifyQuadralateralsThreadedTest.cpp',
        '../tests/PathOpsSimplifyQuadThreadedTest.cpp',
        '../tests/PathOpsSimplifyRectThreadedTest.cpp',
        '../tests/PathOpsSimplifyTest.cpp',
        '../tests/PathOpsSimplifyTrianglesThreadedTest.cpp',
        '../tests/PathOpsTestCommon.cpp',
        '../tests/PathOpsThreadedCommon.cpp',
        '../tests/PathOpsCubicIntersectionTestData.h',
        '../tests/PathOpsExtendedTest.h',
        '../tests/PathOpsQuadIntersectionTestData.h',
        '../tests/PathOpsTestCommon.h',
        '../tests/PathOpsThreadedCommon.h',
        '../tests/Test.cpp',
        '../tests/skia_test.cpp',
        '../tests/Test.h',
      ],
      'dependencies': [
        'skia_base_libs.gyp:skia_base_libs',
        'effects.gyp:effects',
        'flags.gyp:flags',
        'images.gyp:images',
        'utils.gyp:utils',
      ],
      'conditions': [
        [ 'skia_gpu == 1', {
          'include_dirs': [
            '../src/gpu',
          ],
        }],
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
