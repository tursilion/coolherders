//-----------------------------------------------------------------------------
// <copyright file="NetworkErrorScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using System.Diagnostics;
    using System.Globalization;
    using CoolHerders.Properties;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// Specialized message box subclass, used to display network error messages.
    /// </summary>
    public class NetworkErrorScreen : MessageBoxScreen
    {
        #region Initialization

        /// <summary>
        /// Initializes a new instance of the NetworkErrorScreen class
        /// </summary>
        /// <param name="exception">The exception for which we need to generate an error screen</param>
        public NetworkErrorScreen(Exception exception)
            : base(GetErrorMessage(exception), false)
        {
        }

        /// <summary>
        /// Converts a network exception into a user friendly error message.
        /// </summary>
        /// <param name="exception">The exception for which we need to generate an error message.</param>
        /// <returns>The error message of this exception</returns>
        private static string GetErrorMessage(Exception exception)
        {
            Trace.WriteLine(string.Format(CultureInfo.CurrentCulture, "Network operation threw {0}: {1}", exception, exception.Message));

            // Is this a GamerPrivilegeException?
            if (exception is GamerPrivilegeException)
            {
                return Resources.ErrorGamerPrivilege;
            }

            // Is it a NetworkSessionJoinException?
            NetworkSessionJoinException joinException = exception as
                                                            NetworkSessionJoinException;

            if (joinException != null)
            {
                switch (joinException.JoinError)
                {
                    case NetworkSessionJoinError.SessionFull:
                        return Resources.ErrorSessionFull;

                    case NetworkSessionJoinError.SessionNotFound:
                        return Resources.ErrorSessionNotFound;

                    case NetworkSessionJoinError.SessionNotJoinable:
                        return Resources.ErrorSessionNotJoinable;
                }
            }

            // Otherwise just a generic error message.
            return Resources.ErrorNetwork;
        }

        #endregion
    }
}
