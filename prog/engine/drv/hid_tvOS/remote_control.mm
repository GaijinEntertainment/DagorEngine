// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/hid/dag_hiJoystick.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiCreate.h>
#include <drv/hid/dag_hiTvosMap.h>
#include <drv/hid/dag_hiXInputMappings.h>

using namespace HumanInput;

#include "gamepad_ex.h"
#import "remote_control.h"
#include "tvos_joystick_class_driver.h"

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_mathBase.h>

struct TouchPadConfig{
    float centerRadius;
    Point2 center;
    float minSwipeDistance;
    float touchEndDetectValue;
    float maxTapDistance;
};

struct SwipeRecognitionData{
    Point2 startPoint;
    Point2 middlePoint;
    Point2 endPoint;
    float middlePointDist;
};
struct DynamicCursorMotionData
{
    Point2 curPoint;
    Point2 prevPoint;
    double curTime;
    double prevTime;
    Point2 speed;
    float acceleration;
    float maxSpeed;
    Point2 freeDistance;
    bool skip_frame;
    bool free;
    DynamicCursorMotionData();
};

struct GSensorData{
    Point2 deltaAngle;
    Point2 angleZero;
};

typedef NS_ENUM(NSInteger, SwipeDirection){
    SWIPE_DIRECTION_LEFT = 0,
    SWIPE_DIRECTION_RIGHT = 1,
    SWIPE_DIRECTION_UP = 2,
    SWIPE_DIRECTION_DOWN = 3
};

DynamicCursorMotionData::DynamicCursorMotionData()
{
    curPoint.zero();
    prevPoint.zero();
    curTime = 0;
    prevTime = 0;
    speed.zero();
    acceleration = -15;
    maxSpeed = 10;
    freeDistance.zero();
    skip_frame = false;
    free = true;

}

SwipeDirection detectSwipeDirection(Point2 ptStart, Point2 ptEnd)
{
    float xDist = ptEnd.x - ptStart.x;
    float yDist = ptEnd.y - ptStart.y;
    if(fabsf(xDist) < fabsf(yDist))
    {
        //vertical
        if(yDist < .0f)
        {
            //down
            return SWIPE_DIRECTION_DOWN;
        }
        else
        {
            //up
            return SWIPE_DIRECTION_UP;
        }
    }
    else
    {
        //horizontal
        if(xDist < .0f)
        {
            //left
            return SWIPE_DIRECTION_LEFT;
        }
        else
        {
            //right
            return SWIPE_DIRECTION_RIGHT;
        }
    }
}
/******************/
//extern
GCEventViewController * get_EventViewController();


@implementation TvosInputDevices

HumanInput::IGenJoystickClient * joy_client = NULL;

- (id)init
{
   if ( self = [super init] ) {
        _controllers = [[NSMutableArray alloc] init];
       [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(controllerDidConnect:)
                                                 name:GCControllerDidConnectNotification object:nil];
       [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(controllerDidDisconnect:)
                                                 name:GCControllerDidDisconnectNotification object:nil];
       [self reconnectDevices];
   }
   return self;
}

- (void)controllerDidDisconnect:(NSNotification *)notification
{
  [self reconnectDevices];
}

- (void)controllerDidConnect:(NSNotification *)notification
{
  [self reconnectDevices];
}
bool changed;
-(bool) isChanged
{
  if(changed)
  {
    changed = false;
    return true;
  }
  return false;
}

- (void)reconnectDevices
{
    changed = true;
    [_controllers removeAllObjects];

    for(GCController *_siriController in GCController.controllers)
    {
        if(_siriController.microGamepad != nil && _siriController.motion != nil)
        {
            TvosRemoteController *remoteController = [[TvosRemoteController alloc] initWith: _siriController andMenuButtonReceiver: self];
            [remoteController setViewController: get_EventViewController()];

            [_controllers addObject: remoteController];

        }else if (_siriController.extendedGamepad != nil)
       {
            [_controllers insertObject: [[TvosJoystickController alloc] initWith: _siriController.extendedGamepad]  atIndex: 0];
        }else
        {
          debug("TvosInputDevices::reconnectDevices skip unknown device");
        }
    }
    [self updateClient];
 }

 - (void) update
 {
    for( NSObject<TvosInputDeviceUpdate> *controller in _controllers)
    {
        [controller update];
    }

 }

 - (void) setClient:(HumanInput::IGenJoystickClient *) cli
 {
   joy_client = cli;
   [self updateClient];
 }

 - (void) updateClient
 {
    for( NSObject<TvosInputDeviceUpdate> *controller in _controllers)
    {
        [controller getJoystick]->setClient(joy_client);
    }

 }

 - (int) count
 {
   return _controllers.count;
 }

- ( NSObject<TvosInputDeviceUpdate> *) getItem:(int) idx;
{
  return [_controllers objectAtIndex:idx];
}

-(HumanInput::IGenJoystick *) getJoystick
{
  debug("TvosInputDevices::getJoystick - fake - return 0");
  return 0;
}

-(void) menuButtonDetect
{
  if (_controllers.count > 0)
  {
    HumanInput::JoystickRawState &raw_state = const_cast<HumanInput::JoystickRawState &>([[self getItem:0] getJoystick]->getRawState());
    raw_state.buttons.set(TVOS_REMOTE_BUTTON_MENU);
  }
}

@end


@implementation TvosJoystickController

TvosGamepadEx * controler_impl;
- (id)initWith:(GCExtendedGamepad *) extendedGamepad
{
    self = [super init];
    controler_impl = new TvosGamepadEx(extendedGamepad);
    return self;
}

-(void)dealloc
{
    delete controler_impl;
    [super dealloc];
}

- (void) update
{
    controler_impl->update();
}

-(HumanInput::IGenJoystick *) getJoystick
{
  return controler_impl;
}

 @end



@implementation TvosRemoteController

    GCEventViewController *_gcEventViewController;
    GCController *_siriController;
    GCMicroGamepad *_siriControllerGamepad;
    GCMotion *_siriControllerMotion;
    TvosInputDevices *_menuButtonDetector;
    struct TouchPadConfig _touchConfig;
    struct SwipeRecognitionData _swipeData;
    struct GSensorData _gSensorData;
    struct DynamicCursorMotionData _cursorData;

    TvosRemoteControl remote_control;


- (id)   initWith:(GCController *) controller andMenuButtonReceiver: (TvosInputDevices *) menuButtonDetector
    {
        if(self = [super init])
        {
          _menuButtonDetector = menuButtonDetector;
            _siriController = controller;
            _siriControllerGamepad = _siriController.microGamepad;
            _siriControllerMotion = _siriController.motion;
            _siriControllerGamepad.reportsAbsoluteDpadValues = YES;
            _siriControllerMotion.valueChangedHandler = ^(GCMotion *controller)
            {
                //debug("Evgeny:_siriControllerMotion.valueChangedHandler");
                [self recognizeGSensorData:Point3(controller.gravity.x, controller.gravity.y, controller.gravity.z)];
                //debug("Evgeny: siri controller.gravity(%3.1f, %3.1f, %3.1f)", controller.gravity.x, controller.gravity.y, controller.gravity.z);
            };
            _siriControllerGamepad.valueChangedHandler = ^(GCMicroGamepad *gamepad, GCControllerElement *element)
            {

                HumanInput::JoystickRawState &raw_state = const_cast<HumanInput::JoystickRawState &>(remote_control.getRawState());
                if (gamepad.buttonX == element)
                {

                    if(gamepad.buttonX.isPressed)
                        raw_state.buttons.set(TVOS_REMOTE_BUTTON_X);
                    else
                        raw_state.buttons.clr(TVOS_REMOTE_BUTTON_X);
                }
                else if (gamepad.buttonA == element)
                {
                  remote_control.clearClicks();
                  if(gamepad.buttonA.isPressed)
                  {
                    [self clearSwipeData];
                    raw_state.buttons.set([self dpadButtonX:gamepad.dpad.xAxis.value Y:gamepad.dpad.yAxis.value withClick: true] );
                  }
                }
                else if ((gamepad.dpad == element) && !gamepad.buttonA.isPressed)
                {
                    [self dpadDetectX: gamepad.dpad.xAxis.value Y:gamepad.dpad.yAxis.value];
                    [self cursorMotionX: gamepad.dpad.xAxis.value Y:gamepad.dpad.yAxis.value];
                }
            };
            //_gcEventViewController.controllerUserInteractionEnabled = true;
        }
        return self;
    }
-(HumanInput::IGenJoystick *) getJoystick
{
  return &remote_control;
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses
           withEvent:(UIPressesEvent *)event
{
    for(UIPress *press in presses) {

        if(press.type == UIPressTypeMenu) {
           [_menuButtonDetector menuButtonDetect];
        }
    }
    [super pressesBegan:presses withEvent:event];
}

- (void) update
{
   remote_control.update();
   /*Dynamic*/
   double t = CACurrentMediaTime();
   double dt = t - _cursorData.curTime;
   _cursorData.curTime = t;
   if(_cursorData.free){
     if(dt < fabs(_cursorData.speed.length() / _cursorData.acceleration)) {
         Point2 v_norm = normalize(_cursorData.speed);
        _cursorData.freeDistance += (_cursorData.speed + v_norm * _cursorData.acceleration * dt / 2) * dt;

        _cursorData.speed += v_norm * _cursorData.acceleration * dt;
     } else if(_cursorData.speed.x != 0 || _cursorData.speed.y != 0){
       real vx = _cursorData.speed.x;
       real vy = _cursorData.speed.y;

       _cursorData.freeDistance += Point2(vx * vx, vy * vy) / (2 * _cursorData.acceleration);
       _cursorData.speed.x = 0;
       _cursorData.speed.y = 0;

     }
  }
  const real step_y = 0.4;
  const real step_x = 0.6;
  const int max_steps = 1;

  HumanInput::JoystickRawState &raw_state = const_cast<HumanInput::JoystickRawState &>(remote_control.getRawState());
  if(!_cursorData.skip_frame){
    if(_cursorData.freeDistance.y >= step_y )
    {
      do{
        _cursorData.freeDistance.y -= step_y;
      }while(_cursorData.freeDistance.y > max_steps * step_y);

      raw_state.buttons.set(TVOS_REMOTE_MOVE_UP);
      _cursorData.freeDistance.x = 0;
      _cursorData.skip_frame = true;
    }
    else if(_cursorData.freeDistance.y <= -step_y)
    {
      do{
        _cursorData.freeDistance.y += step_y;
      } while(_cursorData.freeDistance.y < -max_steps * step_y);

      raw_state.buttons.set(TVOS_REMOTE_MOVE_DOWN);
      _cursorData.freeDistance.x = 0;
      _cursorData.skip_frame = true;
    }

    if(_cursorData.freeDistance.x >= step_x )
    {
      do{
        _cursorData.freeDistance.x -= step_x;
      }while(_cursorData.freeDistance.x > max_steps * step_x);

      raw_state.buttons.set(TVOS_REMOTE_MOVE_RIGHT);
      _cursorData.freeDistance.y = 0;
      _cursorData.skip_frame = true;
    }
    else if(_cursorData.freeDistance.x <= -step_x)
    {
      do{
        _cursorData.freeDistance.x += step_x;
      }while(_cursorData.freeDistance.x < -max_steps * step_x);

      raw_state.buttons.set(TVOS_REMOTE_MOVE_LEFT);
      _cursorData.freeDistance.y = 0;
      _cursorData.skip_frame = true;
    }

  } else {
    _cursorData.skip_frame = false;
  }
}

-(void) setViewController:(GCEventViewController *) gvc
{
    _gcEventViewController = gvc;
    self.allowedPressTypes = @[[NSNumber numberWithInteger:UIPressTypeMenu]];

    NSMutableArray *gestureRecognizers = [NSMutableArray array];
    [gestureRecognizers addObject:self];
    [gestureRecognizers addObjectsFromArray:_gcEventViewController.view.gestureRecognizers];
    _gcEventViewController.view.gestureRecognizers = gestureRecognizers;

    [self setupControls];
}


-(void) cursorMotionX:(float) x Y:(float) y
{
    if( x != 0 && y!=0 ) {
        if(x == _cursorData.curPoint.x && y == _cursorData.curPoint.y )
            return;
        if(_cursorData.free){
            _cursorData.free = false;
            _cursorData.prevPoint = _cursorData.curPoint = Point2(x, y);
            _cursorData.prevTime = _cursorData.curTime = CACurrentMediaTime();
            _cursorData.speed.zero();
            _cursorData.freeDistance.zero();
        }
        else{
            _cursorData.prevPoint = _cursorData.curPoint;
            _cursorData.prevTime = _cursorData.curTime;
            _cursorData.curPoint = Point2(x, y);
            _cursorData.curTime = CACurrentMediaTime();
            _cursorData.speed = (_cursorData.curPoint - _cursorData.prevPoint)/(_cursorData.curTime-_cursorData.prevTime);
            if(fabs(_cursorData.speed.x)>fabs(_cursorData.speed.y) )
            {
                _cursorData.speed.y = 0;
            }
            else
            {
                _cursorData.speed.x = 0;
            }
            _cursorData.speed.x = clamp(_cursorData.speed.x, -_cursorData.maxSpeed, _cursorData.maxSpeed);
            _cursorData.speed.y = clamp(_cursorData.speed.y, -_cursorData.maxSpeed, _cursorData.maxSpeed);

            _cursorData.freeDistance += _cursorData.curPoint -_cursorData.prevPoint;
        }
    }
    else {
        _cursorData.free = true;
    }
}

-(int) dpadButtonX:(float) x Y:(float) y withClick: (bool) is_click
{
    HumanInput::JoystickRawState &raw_state = const_cast<HumanInput::JoystickRawState &>(remote_control.getRawState());
    static const int padMapping[2][9] =
    {
      {
        TVOS_REMOTE_DPAD_TAP_TOP_LEFT,    TVOS_REMOTE_DPAD_TAP_TOP_CENTER,    TVOS_REMOTE_DPAD_TAP_TOP_RIGHT,
        TVOS_REMOTE_DPAD_TAP_CENTER_LEFT, TVOS_REMOTE_DPAD_TAP_CENTER,        TVOS_REMOTE_DPAD_TAP_CENTER_RIGHT,
        TVOS_REMOTE_DPAD_TAP_BOTTOM_LEFT, TVOS_REMOTE_DPAD_TAP_BUTTOM_CENTER, TVOS_REMOTE_DPAD_TAP_BOTTOM_RIGHT
      },
      {
        TVOS_REMOTE_DPAD_CLICK_TOP_LEFT,    TVOS_REMOTE_DPAD_CLICK_TOP_CENTER,    TVOS_REMOTE_DPAD_CLICK_TOP_RIGHT,
        TVOS_REMOTE_DPAD_CLICK_CENTER_LEFT, TVOS_REMOTE_DPAD_CLICK_CENTER,        TVOS_REMOTE_DPAD_CLICK_CENTER_RIGHT,
        TVOS_REMOTE_DPAD_CLICK_BOTTOM_LEFT, TVOS_REMOTE_DPAD_CLICK_BUTTOM_CENTER, TVOS_REMOTE_DPAD_CLICK_BOTTOM_RIGHT
      }
    };

    float normX = x - _touchConfig.center.x;
    float normY = y - _touchConfig.center.y;
    int buttonIdx;
    if(normX < -_touchConfig.centerRadius)
        buttonIdx = 0;
    else if(normX >_touchConfig.centerRadius)
        buttonIdx = 2;
    else
        buttonIdx = 1;

    if(normY < -_touchConfig.centerRadius)
        buttonIdx += 6;
    else if(normY >_touchConfig.centerRadius)
        buttonIdx += 0;
    else
        buttonIdx += 3;

    return padMapping[is_click ? 1 : 0][buttonIdx];
}

-(void) dpadDetectX:(float) x Y:(float) y
{
    HumanInput::JoystickRawState &raw_state = const_cast<HumanInput::JoystickRawState &>(remote_control.getRawState());
    if(fabsf(x) < _touchConfig.touchEndDetectValue  && fabsf(y) < _touchConfig.touchEndDetectValue)
    {
      //remote_control.clearTaps();
      //touch ends
      //debug("dpadDetectX: dpad gesture");
      [self recognizeDpadGesture];
      [self clearSwipeData];
      return;
    }

    Point2 curPt(x, y);
    _swipeData.endPoint = curPt;

    if(_swipeData.startPoint.x == .0f && _swipeData.startPoint.y == .0f)
    {
        //touch begins
        _swipeData.startPoint  = curPt;
        _swipeData.middlePoint = curPt;

        return;
    }

    //touch in progress
    float curDist = length(_swipeData.startPoint - curPt);
    if(curDist > _swipeData.middlePointDist)
    {
        _swipeData.middlePoint = curPt;
        _swipeData.middlePointDist = curDist;
    }
}

-(void) recognizeDpadGesture
{
    HumanInput::JoystickRawState &raw_state = const_cast<HumanInput::JoystickRawState &>(remote_control.getRawState());
    if(_swipeData.middlePointDist < _touchConfig.minSwipeDistance)
    {
        //too small distance for any swipe
        if(_swipeData.middlePointDist < _touchConfig.maxTapDistance)
        {
          //ok it is tap
          if(_swipeData.endPoint.x != 0 && _swipeData.endPoint.y != 0)
            raw_state.buttons.set([self dpadButtonX: _swipeData.endPoint.x Y: _swipeData.endPoint.y withClick: false] );
        }
        //it is nor tap nor swipe
    }
    else
    {
        if( length(_swipeData.middlePoint - _swipeData.endPoint) < _touchConfig.minSwipeDistance)
        {
            //it is single swipe
            switch (detectSwipeDirection(_swipeData.startPoint, _swipeData.endPoint)) {
                case SWIPE_DIRECTION_LEFT:
                    raw_state.buttons.set(TVOS_REMOTE_DPAD_SWIPE_LEFT);
                    break;
                case SWIPE_DIRECTION_RIGHT:
                    raw_state.buttons.set(TVOS_REMOTE_DPAD_SWIPE_RIGHT);
                    break;
                case SWIPE_DIRECTION_DOWN:
                    raw_state.buttons.set(TVOS_REMOTE_DPAD_SWIPE_DOWN);
                    break;
                case SWIPE_DIRECTION_UP:
                    raw_state.buttons.set(TVOS_REMOTE_DPAD_SWIPE_UP);
                    break;
            }
        }
        else
        {
            //it is double swipe
            switch (detectSwipeDirection(_swipeData.startPoint, _swipeData.middlePoint)) {
                case SWIPE_DIRECTION_LEFT:
                    raw_state.buttons.set(TVOS_REMOTE_DPAD_SWIPE_LEFT_RIGHT);
                    break;
                case SWIPE_DIRECTION_RIGHT:
                    raw_state.buttons.set(TVOS_REMOTE_DPAD_SWIPE_RIGHT_LEFT);
                    break;
                case SWIPE_DIRECTION_DOWN:
                    raw_state.buttons.set(TVOS_REMOTE_DPAD_SWIPE_DOWN_UP);
                    break;
                case SWIPE_DIRECTION_UP:
                    raw_state.buttons.set(TVOS_REMOTE_DPAD_SWIPE_UP_DOWN);
                    break;
            }
        }
    }
}

-(void) recognizeGSensorData:(Point3) gravity
{
    {
        HumanInput::JoystickRawState &raw_state = const_cast<HumanInput::JoystickRawState &>(remote_control.getRawState());

        float s = sin(_gSensorData.angleZero.y);
        float c = cos(_gSensorData.angleZero.y);

        float x = gravity.x;
        float y = c * gravity.y - s * gravity.z;
        //float z = s * gravity.y + c * gravity.z;

        float yz = safe_sqrt(1 - x * x);
        float xz = safe_sqrt(1 - y * y);
        //float xy = safe_sqrt(1 - z * z);

        float ax = safe_atan2(x, yz);
        float ay = 2*safe_atan2(y, xz);
        //float az = safe_atan2(z, xy);

        //debug("Gravity angle:(%4.0f, %4.0f, %4.0f)o", ax/PI*180, ay/PI*180, az/PI*180);


        raw_state.sensorX = ax;
        raw_state.sensorY = ay;

        raw_state.buttons.clr(TVOS_REMOTE_MOVEMENT_BACK);
        raw_state.buttons.clr(TVOS_REMOTE_MOVEMENT_FORWARD);
        if (ay < - _gSensorData.deltaAngle.y)
        {
          //moveBack
          raw_state.buttons.set(TVOS_REMOTE_MOVEMENT_BACK);
        }
        else if (ay > _gSensorData.deltaAngle.y)
        {
          //moveForward
          raw_state.buttons.set(TVOS_REMOTE_MOVEMENT_FORWARD);
        }
        else
        {
            //stand
        }

        raw_state.buttons.clr(TVOS_REMOTE_ROTATION_LEFT);
        raw_state.buttons.clr(TVOS_REMOTE_ROTATION_RIGHT);
        if (ax < -_gSensorData.deltaAngle.x)
        {
          //turnLeft
          raw_state.buttons.set(TVOS_REMOTE_ROTATION_LEFT);
        }
        else if (ax > _gSensorData.deltaAngle.x)
        {
            //turnRight
            raw_state.buttons.set(TVOS_REMOTE_ROTATION_RIGHT);
        }
        else
        {
            //straight
        }
    }
}

-(void) clearSwipeData
{
    _swipeData.startPoint.zero();
    _swipeData.middlePoint.zero();
    _swipeData.endPoint.zero();
    _swipeData.middlePointDist = .0f;
}

-(void) setupControls
{
    [self clearSwipeData];

    _gSensorData.deltaAngle.set(15.f*PI/180, 15.f*PI/180);
    _gSensorData.angleZero.set(0, 60.f*PI/180);
    _touchConfig.center.set(0.0f, -0.1f);
    _touchConfig.centerRadius = 0.6f;
    _touchConfig.maxTapDistance = 0.02f;
    _touchConfig.minSwipeDistance = .8f;
    _touchConfig.touchEndDetectValue = 0.0001f;
}
@end
