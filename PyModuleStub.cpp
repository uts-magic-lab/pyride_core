//
//  PyModuleStub.cpp
//  PyRIDE
//
//  Created by Xun Wang on 16/12/13.
//  Copyright 2013 Galaxy Network. All rights reserved.
//
#include "PyModuleStub.h"

namespace pyride {

#if PY_MAJOR_VERSION >= 3
  #define PyInt_AsLong PyLong_AsLong
  #define PyInt_Check PyLong_Check
#endif

std::vector<long> g_PyModuleTimerList;

//#pragma mark PyModuleExtension implmentation
PyModuleExtension::PyModuleExtension( const char * name ) :
  clientID_( 0 ),
  pow_( NULL ),
  pPyModule_( NULL ),
  pyModuleCommandHandler_( NULL )
{
  name_ = name;
}

PyModuleExtension::~PyModuleExtension()
{
  this->fini();
}

PyObject * PyModuleExtension::init( PyOutputWriter * pow )
{
  pPyModule_ = this->createPyModule();

  if (!pPyModule_)
    return NULL;
  
  pow_ = pow;
  
  pyModuleCommandHandler_ = new PyModuleExtendedCommandHandler( this );
  ServerDataProcessor::instance()->addCommandHandler( pyModuleCommandHandler_ );
  this->clientID( ServerDataProcessor::instance()->clientID() );
  
  PyObject * arg = NULL;
  if (ServerDataProcessor::instance()->teamColour() == BlueTeam) {
    arg = Py_BuildValue( "s", "blue" );
  }
  else {
    arg = Py_BuildValue( "s", "pink" );
  }
  PyObject_SetAttrString( pPyModule_, "TeamColour", arg );
  //PyModule_AddObject( pyModule, "TeamColour", arg );
  
  Py_DECREF( arg );
  
  arg = Py_BuildValue( "i", ServerDataProcessor::instance()->teamMemberID() );
  PyObject_SetAttrString( pPyModule_, "MemberID", arg );
  //PyModule_AddObject( pyModule, "MemberID", arg );
  Py_DECREF( arg );
  
  return pPyModule_;
}

void PyModuleExtension::fini()
{
  if (pPyModule_) {
    for (size_t i = 0; i < g_PyModuleTimerList.size(); i++) {
      ServerDataProcessor::instance()->delTimer( g_PyModuleTimerList[i] );
    }
    g_PyModuleTimerList.clear();
    
    ServerDataProcessor::instance()->removeCommandHandler( pyModuleCommandHandler_ );
    delete pyModuleCommandHandler_;
    pyModuleCommandHandler_ = NULL;
    pow_ = NULL;

    Py_DECREF( pPyModule_ );
    pPyModule_ = NULL;
  }
}

void PyModuleExtension::write( const char * str )
{
  if (!pow_)
    return;
  
  pow_->write( str );
}

void PyModuleExtension::sendTeamMessage( const char * mesg )
{
  if (!pow_)
    return;

  if (!mesg || strlen( mesg ) == 0)
    return;
  
  pow_->broadcastMessage( mesg );
}

void PyModuleExtension::clientID( char cID )
{
  if (!pPyModule_)
    return;

  clientID_ = cID;
  
  TeamColour teamColour = (TeamColour)(cID & 0xf);
  int teamID = (cID >> 4) & 0xf;
  
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  PyObject * arg = NULL;
  if (teamColour == BlueTeam) {
    arg = Py_BuildValue( "s", "blue" );
  }
  else {
    arg = Py_BuildValue( "s", "pink" );
  }
  PyObject_SetAttrString( pPyModule_, "TeamColour", arg );
  
  Py_DECREF( arg );
  
  arg = Py_BuildValue( "i", teamID );
  PyObject_SetAttrString( pPyModule_, "MemberID", arg );
  Py_DECREF( arg );
  
  PyGILState_Release( gstate );
}

void PyModuleExtension::setTeamColour( TeamColour teamColour )
{
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  PyObject * arg = NULL;
  
  if (teamColour == BlueTeam) {
    arg = Py_BuildValue( "s", "blue" );
  }
  else {
    arg = Py_BuildValue( "s", "pink" );
  }
  PyObject_SetAttrString( pPyModule_, "TeamColour", arg );
  
  Py_DECREF( arg );
  PyGILState_Release( gstate );
  ServerDataProcessor::instance()->setTeamColour( teamColour );
  clientID_ = (clientID_ & 0xf0) | (teamColour & 0xf);
}

bool PyModuleExtension::invokeCallback( const char * fnName, PyObject * arg )
{
  bool retval = false;

  if (!pPyModule_)
    return retval;
  
  //DEBUG_MSG( "Attempt get callback function %s\n", fnName );
  PyObject * callbackFn = PyObject_GetAttrString( pPyModule_, const_cast<char *>(fnName) );
  if (!callbackFn || callbackFn == Py_None) {
    PyErr_Clear();
    return retval;
  }
  else if (!PyCallable_Check( callbackFn )) {
    PyErr_Format( PyExc_TypeError, "%s is not callable object", fnName );
  }
  else {
    retval = true; // as long as there is a function link to the callback, we consider it is rightly consumed.
    PyObject * pResult = PyObject_CallObject( callbackFn, arg );
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
    Py_XDECREF( pResult );
  }
  Py_DECREF( callbackFn );
  return retval;
}

bool PyModuleExtension::invokeCallback( const char * fnName, PyObject * arg, PyObject * & result )
{
  bool retval = false;
  result = NULL;

  if (!pPyModule_)
    return retval;

  //DEBUG_MSG( "Attempt get callback function %s\n", fnName );

  PyObject * callbackFn = PyObject_GetAttrString( pPyModule_, const_cast<char *>(fnName) );
  if (!callbackFn) {
    PyErr_Clear();
    return retval;
  }
  else if (!PyCallable_Check( callbackFn )) {
    PyErr_Format( PyExc_TypeError, "%s is not callable object", fnName );
  }
  else {
    retval = true;
    PyObject * pResult = PyObject_CallObject( callbackFn, arg );
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
    else {
      result = pResult; // caller must decrement reference
    }
  }
  Py_DECREF( callbackFn );
  return retval;
}

// internal helper functions
void PyModuleExtension::swapCallbackHandler( PyObject * & master, PyObject * newObj )
{
  if (newObj) {
    if (master) {
      Py_DECREF( master );
    }
    master = newObj;
    Py_INCREF( master );
  }
  else if (master) {
    Py_DECREF( master );
    master = NULL;
  }
}

void PyModuleExtension::invokeCallbackHandler( PyObject * & cbObj, PyObject * arg )
{
  if (cbObj) {
    PyObject * pResult = PyObject_CallObject( cbObj, arg );
    if (PyErr_Occurred()) {
      PyErr_Print();
    }
    Py_XDECREF( pResult );
  }
}

//#pragma mark PyModuleExtendedCommandHandler implementation
PyModuleExtendedCommandHandler::PyModuleExtendedCommandHandler( PyModuleExtension * pyExtModule )
{
  pyExtModule_ = pyExtModule;
}

/** @name Remote Client Access Functions
 *
 */
/*! \typedef onRemoteCommand(cmd_id, cmd_text)
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when a custom command is received from a remote client.
 *  \param int cmd_id. Custom command ID.
 *  \param str cmd_text. Custom command text string.
 *  \return None.
 *  \note The remote client must take the exclusive control of the robot before
 *   its commands can trigger this callback function.
 */
bool PyModuleExtendedCommandHandler::executeRemoteCommand( PyRideExtendedCommand command, int & retVal,
                                                       const unsigned char * optionalData,
                                                       const int optionalDataLength )
{
  bool success = false;
  retVal = 0;

  if (!pyExtModule_)
    return success;
  
  PyObject * arg = NULL;
  PyObject * result = NULL;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  if (optionalData) {
    std::string data( (char *)optionalData, optionalDataLength );
    arg = Py_BuildValue( "(is)", (int) command, data.c_str() );
  }
  else {
    arg = Py_BuildValue( "(is)", (int) command, "" );
  }

  success = pyExtModule_->invokeCallback( "onRemoteCommand", arg, result );

  if (result) {
    if (PyBool_Check( result )) {
      retVal = PyObject_IsTrue( result );
    }
    else if (PyLong_Check( result )) {
      retVal = PyLong_AsLong( result );
    }
    else {
      PyErr_Format( PyExc_TypeError,
          "onRemoteCommand callback return value should be an integer or boolean" );
    }
  }
  Py_DECREF( arg );
  Py_XDECREF( result );
  
  PyGILState_Release( gstate );
  
  return success;
}

void PyModuleExtendedCommandHandler::cancelCurrentOperation()
{
  if (!pyExtModule_)
    return;
  
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  pyExtModule_->invokeCallback( "onCurrentOperationCanceled", NULL );
  
  PyGILState_Release( gstate );
}

/*! \typedef onUserLogOn(user_name)
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when a user logs in through a remote client.
 *  \param str user_name. The name of the user.
 *  \return None or boolean. When the return value is false, the user will be
 *  removed from the system; otherwise the user is allowed to use the robot.
 */
bool PyModuleExtendedCommandHandler::onUserLogOn( const std::string & username )
{
  if (!pyExtModule_)
    return true;

  PyObject * arg = NULL;
  PyObject * result = NULL;
  bool retVal = true; //make it more permissive.

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  arg = Py_BuildValue( "(s)", username.c_str() );

  pyExtModule_->invokeCallback( "onUserLogOn", arg, result );
  if (result && PyBool_Check( result )) {
    retVal = PyObject_IsTrue( result );
  }
  Py_DECREF( arg );
  Py_XDECREF( result );

  PyGILState_Release( gstate );

  return retVal;
}

/*! \typedef onUserLogOff(user_name)
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when a remote user logs off.
 *  \param str user_name. The name of the user.
 *  \return None.
 */
void PyModuleExtendedCommandHandler::onUserLogOff( const std::string & username )
{
  if (!pyExtModule_)
    return;

  PyObject * arg = NULL;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  arg = Py_BuildValue( "(s)", username.c_str() );

  pyExtModule_->invokeCallback( "onUserLogOff", arg );
  Py_DECREF( arg );

  PyGILState_Release( gstate );

}

/*! \typedef onExclusiveControlRequest(user_name)
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when a user is requesting the exclusive
 *  control of the robot through a remote client.
 *  \param str user_name. The name of the user.
 *  \return None or an integer. When the return value is a non-zero
 *  integer, the user request is rejected; otherwise, the user will
 *  gain the exclusive control right.
 */
int PyModuleExtendedCommandHandler::onExclusiveCtrlRequest( const std::string & username )
{
  if (!pyExtModule_)
    return 0;
  
  PyObject * arg = NULL;
  PyObject * result = NULL;
  int retVal = 0; //make it more permissive.

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  arg = Py_BuildValue( "(s)", username.c_str() );

  pyExtModule_->invokeCallback( "onExclusiveControlRequest", arg, result );
  if (result && PyInt_Check( result )) {
    retVal = (int)PyInt_AsLong( result );
  }
  Py_DECREF( arg );
  Py_XDECREF( result );

  PyGILState_Release( gstate );

  return retVal;
}

/*! \typedef onExclusiveControlRelease(user_name)
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when a remote user releases the exclusive control.
 *  \param str user_name. The name of the user.
 *  \return None.
 */
/**@}*/
void PyModuleExtendedCommandHandler::onExclusiveCtrlRelease( const std::string & username )
{
  if (!pyExtModule_)
    return;
  
  PyObject * arg = NULL;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  arg = Py_BuildValue( "(s)", username.c_str() );

  pyExtModule_->invokeCallback( "onExclusiveControlRelease", arg );
  Py_DECREF( arg );

  PyGILState_Release( gstate );
}

/** @name Timer Management Functions
 *
 */
/**@{*/
/*! \typedef onTimer(timer_id)
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when a timer object (created by ROBOT_MODEL_DOXYGEN.addTimer) is fired.
 *  \param int timer_id. ID of the timer object.
 *  \return None.
 */
void PyModuleExtendedCommandHandler::onTimer( const long timerID )
{
  if (!pyExtModule_)
    return;
  
  std::string username;
  PyObject * arg = NULL;
  
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  arg = Py_BuildValue( "(i)", timerID );
  
  pyExtModule_->invokeCallback( "onTimer", arg );
  Py_DECREF( arg );
  
  PyGILState_Release( gstate );
}

/*! \typedef onTimerLapsed(timer_id)
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when a timer object (created by ROBOT_MODEL_DOXYGEN.addTimer) is fired for the last time.
 *  \param int timer_id. ID of the timer object.
 *  \return None.
 *  \note This callback function works only on timers with limited life span.
 */
/**@}*/
void PyModuleExtendedCommandHandler::onTimerLapsed( const long timerID )
{
  if (!pyExtModule_)
    return;
  
  std::string username;
  PyObject * arg = NULL;
  
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  arg = Py_BuildValue( "(i)", timerID );
  
  pyExtModule_->invokeCallback( "onTimerLapsed", arg );
  Py_DECREF( arg );
  
  PyGILState_Release( gstate );
}

/** @name Miscellaneous Functions
 *
 */
/**@{*/
/*! \typedef onPeerMessage( sender_id, message )
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when a team message is received from a team member (calling ROBOT_MODEL_DOXYGEN.sendTeamMessage).
 *  \param int sender_id. The ID of the team member who sends the message.
 *  \param str message. Text of the message.
 *  \return None.
 */
/**@}*/
/**@{*/
/*! \typedef onSnapshotImage(image_name)
 *  \memberof ROBOT_MODEL_DOXYGEN.
 *  \brief Callback function when ROBOT_MODEL_DOXYGEN.takeCameraSnapshot is called.
 *  \param str image_name. Path for the saved image.
 *  \return None.
 */
/**@}*/
void PyModuleExtendedCommandHandler::onSnapshotImage( const string & name )
{
  if (!pyExtModule_)
    return;
  
  std::string username;
  PyObject * arg = NULL;
  
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  
  arg = Py_BuildValue( "(s)", name.c_str() );
  
  pyExtModule_->invokeCallback( "onSnapshotImage", arg );
  Py_DECREF( arg );
  
  PyGILState_Release( gstate );
}
} // namespace pyride
