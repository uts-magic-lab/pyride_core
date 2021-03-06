//
//  ServerDataProcessor.h
//  PyRIDE
//
//  Created by Xun Wang on 10/05/10.
//  Copyright 2010 GalaxyNetwork. All rights reserved.
//

#ifndef ServerDataProcessor_h_DEFINED
#define ServerDataProcessor_h_DEFINED

#include <vector>
#include <string>
#include "PyRideNetComm.h"
#ifdef IOS_BUILD
#include "iOSAppConfigManager.h"
#else
#include "AppConfigManager.h"
#endif

namespace pyride {

typedef std::vector< FieldObject > ObservedObjects;

class ServerDataProcessor;

class PyRideExtendedCommandHandler : public VideoDeviceDataHandler
{
protected:
  virtual bool executeRemoteCommand( PyRideExtendedCommand command, int & retVal,
                                    const unsigned char * optinalData = NULL,
                                    const int optionalDataLength = 0 ) = 0;
  virtual void cancelCurrentOperation() = 0;
  virtual bool onUserLogOn( const std::string & name ) { return false; }
  virtual void onUserLogOff( const std::string & name ) {}

  virtual int onExclusiveCtrlRequest( const std::string & name ) { return 0; }
  virtual void onExclusiveCtrlRelease( const std::string & name ) {}

  virtual void onTimer( const long timerID ) {}
  virtual void onTimerLapsed( const long timerID ) {}

  virtual void onTelemetryStreamControl( bool isStart ) {};
  
  friend class ServerDataProcessor;
};

class ServerDataProcessor : public RobotDataHandler, VideoDeviceDataHandler
{
public:
  static ServerDataProcessor * instance();
  ~ServerDataProcessor();

  void init( const VideoDeviceList & videoObjs, const AudioDeviceList & audioObjs );
  void fini();
  void addCommandHandler( PyRideExtendedCommandHandler * cmdHandler = NULL );
  void removeCommandHandler( PyRideExtendedCommandHandler * cmdHandler = NULL );
  
  void discoverConsoles();
  void disconnectConsole( SOCKET_T fd );
  void disconnectConsoles();

  void setClientID( const char clientID ) { clientID_ = clientID; }
  void setTeamMemberID( int number = 1, TeamColour team = BlueTeam );
  void setTeamColour( TeamColour team );
  void updateRobotTelemetry( float x, float y, float heading );
  void updateRobotTelemetryWithDefault();
  void updateRobotTelemetry( RobotPose & pos, ObservedObjects & objects );

  void blockRemoteExclusiveControl( bool isyes );
  
  void takeCameraSnapshot( bool takeAllCamera );
  int getMyIPAddress();
  
  int activeVideoObjectList( std::vector<std::string> & namelist );
  bool dispatchVideoDataTo( int vidObjID, struct sockaddr_in & cAddr,
                            short port, bool todispath );
  bool dispatchAudioDataTo( struct sockaddr_in & cAddr,
                            short port, bool todispath );
  bool setCameraParameter( int vidObjID, int id_idx, int value );
  
  void updateOperationalStatus( RobotOperationalState status, const char * optionalData = NULL,
                                  const int optionalDataLength = 0 );  
  TeamColour teamColour() { return (TeamColour)(clientID_ & 0xf); }
  int teamMemberID() { return (clientID_ >> 4); }
  const RobotInfo & defaultRobotInfo() { return defaultRobotInfo_; }
  void setDefaultRobotInfo( const RobotType rtype, const RobotPose & pose, const RobotCapability & capabilities )
  {
    defaultRobotInfo_.type = rtype;
    defaultRobotInfo_.pose = pose;
    defaultRobotInfo_.capabilities = capabilities;
  }

  long addTimer( float initialTime, long repeats = 0, float interval = 1.0 );
  void delTimer( long tID );
  void delAllTimers();
  long totalTimers();
  bool isTimerRunning( long tID );
  bool isTimerExecuting( long tID );

private:
  typedef std::vector<PyRideExtendedCommandHandler *> PyRideExtendedCommandHandlerList;
  PyRideNetComm * pNetComm_;
  PyRideExtendedCommandHandlerList cmdHandlerList_;
  VideoDeviceList * activeVideoObjs_;
  AudioDeviceList * activeAudioObjs_;

  RobotInfo defaultRobotInfo_;
  
  static ServerDataProcessor * s_pServerDataProcessor;

  ServerDataProcessor();

  // RobotDataHandler
  bool executeRemoteCommand( const unsigned char * commandData, const int dataLength, int & retVal );
  void cancelCurrentOperation();
  bool onUserLogOn( const unsigned char * authCode, SOCKET_T fd, struct sockaddr_in & addr );
  void onUserLogOff( SOCKET_T fd );

  int onExclusiveCtrlRequest( SOCKET_T fd );
  void onExclusiveCtrlRelease( SOCKET_T fd );

  void onTimer( const long timerID );
  void onTimerLapsed( const long timerID );
  
  void onTelemetryStreamControl( bool isStart );
  void onSnapshotImage( const string & imageName );
};
} // namespace pyride
#endif  // ServerDataProcessor_h_DEFINED
