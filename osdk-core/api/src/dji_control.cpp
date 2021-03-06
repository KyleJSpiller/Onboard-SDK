/** @file dji_control.cpp
 *  @version 3.3
 *  @date April 2017
 *
 *  @brief
 *  Control API for DJI OSDK library
 *
 *  @copyright 2017 DJI. All rights reserved.
 *
 */

#include "dji_control.hpp"
#include "dji_vehicle.hpp"

using namespace DJI;
using namespace DJI::OSDK;

Control::Control(Vehicle* vehicle)
  : vehicle(vehicle)
  , wait_timeout(10)
{
}

Control::~Control()
{
}

void
Control::action(const int cmd, VehicleCallBack callback, UserData userData)
{
  uint8_t data    = cmd;
  int     cbIndex = vehicle->callbackIdIndex();
  if (callback)
  {
    vehicle->nbCallbackFunctions[cbIndex] = (void*)callback;
    vehicle->nbUserData[cbIndex]          = userData;
  }
  else
  {
    // Support for default callbacks
    vehicle->nbCallbackFunctions[cbIndex] = (void*)actionCallback;
    vehicle->nbUserData[cbIndex]          = NULL;
  }
  vehicle->protocolLayer->send(2, DJI::OSDK::encrypt,
                               OpenProtocol::CMDSet::Control::task, &data,
                               sizeof(data), 500, 2, true, cbIndex);
}

ACK::ErrorCode
Control::action(const int cmd, int timeout)
{
  ACK::ErrorCode ack;
  uint8_t        data = cmd;

  vehicle->protocolLayer->send(2, DJI::OSDK::encrypt,
                               OpenProtocol::CMDSet::Control::task, &data,
                               sizeof(data), 500, 2, false, 2);

  ack = *((ACK::ErrorCode*)vehicle->waitForACK(
    OpenProtocol::CMDSet::Control::task, timeout));

  return ack;
}

ACK::ErrorCode
Control::armMotors(int wait_timeout)
{
  return this->action(FlightCommand::startMotor, wait_timeout);
}

void
Control::armMotors(VehicleCallBack callback, UserData userData)
{
  this->action(FlightCommand::startMotor, callback, userData);
}

ACK::ErrorCode
Control::disArmMotors(int wait_timeout)
{
  return this->action(FlightCommand::stopMotor, wait_timeout);
}

void
Control::disArmMotors(VehicleCallBack callback, UserData userData)
{
  this->action(FlightCommand::stopMotor, callback, userData);
}

ACK::ErrorCode
Control::takeoff(int wait_timeout)
{
  return this->action(FlightCommand::takeOff, wait_timeout);
}

void
Control::takeoff(VehicleCallBack callback, UserData userData)
{
  this->action(FlightCommand::takeOff, callback, userData);
}

ACK::ErrorCode
Control::goHome(int wait_timeout)
{
  return this->action(FlightCommand::goHome, wait_timeout);
}

void
Control::goHome(VehicleCallBack callback, UserData userData)
{
  this->action(FlightCommand::goHome, callback, userData);
}

ACK::ErrorCode
Control::land(int wait_timeout)
{
  return this->action(FlightCommand::landing, wait_timeout);
}

void
Control::land(VehicleCallBack callback, UserData userData)
{
  this->action(FlightCommand::landing, callback, userData);
}

void
Control::flightCtrl(CtrlData data)
{
  vehicle->protocolLayer->send(
    0, DJI::OSDK::encrypt, OpenProtocol::CMDSet::Control::control,
    static_cast<void*>(&data), sizeof(CtrlData), 500, 2, false, 1);
}

void
Control::flightCtrl(AdvancedCtrlData data)
{
  vehicle->protocolLayer->send(
    0, DJI::OSDK::encrypt, OpenProtocol::CMDSet::Control::control,
    static_cast<void*>(&data), sizeof(AdvancedCtrlData), 500, 2, false, 1);
}

void
Control::positionAndYawCtrl(float32_t x, float32_t y, float32_t z,
                            float32_t yaw)
{
  //! @note 145 is the flag value of this mode
  uint8_t ctrl_flag = (VERTICAL_POSITION | HORIZONTAL_POSITION | YAW_ANGLE |
                       HORIZONTAL_GROUND | STABLE_ENABLE);
  CtrlData data(ctrl_flag, x, y, z, yaw);

  return this->flightCtrl(data);
}

void
Control::velocityAndYawRateCtrl(float32_t Vx, float32_t Vy, float32_t Vz,
                                float32_t yawRate)
{
  //! @note 72 is the flag value of this mode
  uint8_t ctrl_flag =
    (VERTICAL_VELOCITY | HORIZONTAL_VELOCITY | YAW_RATE | HORIZONTAL_GROUND);
  CtrlData data(ctrl_flag, Vx, Vy, Vz, yawRate);

  return this->flightCtrl(data);
}

void
Control::attitudeAndVertPosCtrl(float32_t roll, float32_t pitch, float32_t yaw,
                                float32_t z)
{
  //! @note 18 is the flag value of this mode
  uint8_t ctrl_flag =
    (VERTICAL_POSITION | HORIZONTAL_ANGLE | YAW_ANGLE | HORIZONTAL_BODY);
  CtrlData data(ctrl_flag, roll, pitch, z, yaw);

  return this->flightCtrl(data);
}

void
Control::angularRateAndVertPosCtrl(float32_t rollRate, float32_t pitchRate,
                                   float32_t yawRate, float32_t z)
{
  //! @note 218 is the flag value of this mode
  uint8_t ctrl_flag =
    (VERTICAL_POSITION | HORIZONTAL_ANGULAR_RATE | YAW_RATE | HORIZONTAL_BODY);
  CtrlData data(ctrl_flag, rollRate, pitchRate, z, yawRate);

  return this->flightCtrl(data);
}

void
Control::emergencyBrake()
{
  //! @note 75 is the flag value of this mode
  AdvancedCtrlData data(72, 0, 0, 0, 0, 0, 0);

  return this->flightCtrl(data);
}

void
Control::actionCallback(Vehicle* vehiclePtr, RecvContainer recvFrame,
                        UserData userData)
{
  ACK::ErrorCode ack;
  Control*       controlPtr = vehiclePtr->control;

  if (recvFrame.recvInfo.len - Protocol::PackageMin <= sizeof(uint16_t))
  {

    ack.info = recvFrame.recvInfo;
    ack.data = recvFrame.recvData.ack;

    if (ACK::getError(ack))
    {
      ACK::getErrorCodeMessage(ack, __func__);
    }
  }
  else
  {
    DERROR("ACK is exception, sequence %d\n", recvFrame.recvInfo.seqNumber);
  }
}
Control::CtrlData::CtrlData(uint8_t in_flag, float32_t in_x, float32_t in_y,
                            float32_t in_z, float32_t in_yaw)
  : flag(in_flag)
  , x(in_x)
  , y(in_y)
  , z(in_z)
  , yaw(in_yaw)
{
}

Control::AdvancedCtrlData::AdvancedCtrlData(uint8_t in_flag, float32_t in_x,
                                            float32_t in_y, float32_t in_z,
                                            float32_t in_yaw, float32_t x_forw,
                                            float32_t y_forw)
  : flag(in_flag)
  , x(in_x)
  , y(in_y)
  , z(in_z)
  , yaw(in_yaw)
  , xFeedforward(x_forw)
  , yFeedforward(y_forw)
  , advFlag(0x01)
{
}
