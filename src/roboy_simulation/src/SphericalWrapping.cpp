#include "SphericalWrapping.hpp"

using namespace roboy_simulation;

SphericalWrapping::SphericalWrapping(): IViaPoints(math::Vector3(0,0,0), Type::SPHERICAL, nullptr), stateMachine(StateMachine()),
                                    radius(0), prevCoord(math::Vector3(0,0,0)), nextCoord(math::Vector3(0,0,0)),
                                    normal(math::Vector3(0,0,0)), arcAngle(0)
{

};

SphericalWrapping::SphericalWrapping(math::Vector3 point, physics::LinkPtr link): SphericalWrapping()
{
    localCoordinates = point;
    this->link = link;
};

SphericalWrapping::SphericalWrapping(math::Vector3 point, double radius, int state, int counter, physics::LinkPtr link): SphericalWrapping(point, link)
{
    this->radius = radius;
    this->stateMachine.state = (StateMachine::State) state;
    this->stateMachine.revCounter = counter;
};

void SphericalWrapping::UpdateForcePoints()
{
    prevCoord = prevPoint->nextForcePoint;
    nextCoord = nextPoint->prevForcePoint;

    stateMachine.UpdateState(prevCoord, nextCoord, globalCoordinates, radius);

    //if muscle is not wrapping, use straight line calculation
    if (stateMachine.state == StateMachine::NOTWRAPPING)
    {
        prevForcePoint = globalCoordinates;
        nextForcePoint = globalCoordinates;
        previousSegmentLength = (prevCoord - prevForcePoint).GetLength();
    }

    //compute tangent points
    //compute unit vectors and according length
    double l_j1 = (prevCoord-this->globalCoordinates).GetLength();
    math::Vector3 j1 = (prevCoord-this->globalCoordinates)/l_j1;
    double l_j2 = (nextCoord-this->globalCoordinates).GetLength();
    math::Vector3 j2 = (nextCoord-this->globalCoordinates)/l_j2;

    //compute normal,
    normal = j1.Cross(j2);

    //compute k1, k2
    math::Vector3 k1 = j1.Cross(normal);
    math::Vector3 k2 = normal.Cross(j2);

    //compute length of a1, a2, b1, b2
    double a1 = radius*radius/l_j1;
    double a2 = radius*radius/l_j2;
    double b1 = sqrt(radius*radius - a1*a1);
    double b2 = sqrt(radius*radius - a2*a2);

    if (stateMachine.state == StateMachine::POSITIVE)
    {
        this->prevForcePoint = this->globalCoordinates + a1*j1 - b1*k1;
        this->nextForcePoint = this->globalCoordinates + a2*j2 - b2*k2;
    }
    else if (stateMachine.state == StateMachine::NEGATIVE)
    {
        this->prevForcePoint = this->globalCoordinates + a1*j1 + b1*k1;
        this->nextForcePoint = this->globalCoordinates + a2*j2 + b2*k2;
    }

    //update revolution counter
    double projection = (prevCoord - this->prevForcePoint).Dot(this->nextForcePoint - this->prevForcePoint);
    stateMachine.UpdateRevCounter(projection);

    //calculate the wrapping angle
    double angle = acos(1 - (this->prevForcePoint-this->nextForcePoint).GetLength()/2*radius*radius);
    arcAngle = 2*(boost::math::constants::pi<double>())*ceil(stateMachine.revCounter/2);
    arcAngle += (stateMachine.revCounter % 2 == 0)?(angle):(-angle);

    //calculate the lines of action and the muscle's length
    previousSegmentLength = (prevCoord - this->prevForcePoint).GetLength() + arcAngle*radius + (this->nextForcePoint - nextCoord).GetLength();
};
