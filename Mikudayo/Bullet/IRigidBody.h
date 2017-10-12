#pragma once

enum ShapeType {
    kUnknownShape = -1,
    kSphereShape,
    kBoxShape,
    kCapsuleShape,
    kConeShape,
    kCylinderShape,
    kPlaneShape,
    kMaxShapeType
};

enum ObjectType {
    kStaticObject,
    kDynamicObject,
    kAlignedObject,
    kMaxObjectType
};
