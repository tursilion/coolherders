//-----------------------------------------------------------------------------
// <copyright file="CreateOrFindSessionScreen.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Networking
{
    #region Using Statements
    using System;
    using CoolHerders.Properties;
    using CoolHerders.Screens;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;
    #endregion

    /// <summary>
    /// This menu screen lets the user choose whether to create a new
    /// network session, or search for an existing session to join.
    /// </summary>
    internal class CreateOrFindSessionScreen : MenuScreen
    {
        #region Fields

        /// <summary>
        /// The network session type we are creating
        /// </summary>
        private NetworkSessionType sessionType;

        #endregion

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the CreateOrFindSessionScreen class.
        /// </summary>
        /// <param name="sessionType">The type of network session that a menu should be generated for</param>
        public CreateOrFindSessionScreen(NetworkSessionType sessionType)
            : base(GetMenuTitle(sessionType))
        {
            this.sessionType = sessionType;

            // Create our menu entries.
            MenuEntry createSessionMenuEntry = new MenuEntry(Resources.CreateSession);
            MenuEntry findSessionsMenuEntry = new MenuEntry(Resources.FindSessions);
            MenuEntry backMenuEntry = new MenuEntry(Resources.Back);

            // Hook up menu event handlers.
            createSessionMenuEntry.Selected += this.CreateSessionMenuEntrySelected;
            findSessionsMenuEntry.Selected += this.FindSessionsMenuEntrySelected;
            backMenuEntry.Selected += OnCancel;

            // Add entries to the menu.
            MenuEntries.Add(createSessionMenuEntry);
            MenuEntries.Add(findSessionsMenuEntry);
            MenuEntries.Add(backMenuEntry);
        }

        /// <summary>
        /// Helper chooses an appropriate menu title for the specified session type.
        /// </summary>
        /// <param name="sessionType">The type of network session for which we need a menu title</param>
        /// <returns>The menu title for this network session selection menu</returns>
        private static string GetMenuTitle(NetworkSessionType sessionType)
        {
            switch (sessionType)
            {
                case NetworkSessionType.PlayerMatch:
                    return Resources.PlayerMatch;

                case NetworkSessionType.SystemLink:
                    return Resources.SystemLink;

                default:
                    throw new NotSupportedException();
            }
        }

        #endregion

        #region Event Handlers

        /// <summary>
        /// Event handler for when the Create Session menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event args</param>
        private void CreateSessionMenuEntrySelected(object sender, EventArgs e)
        {
            try
            {
                // Begin an asynchronous create network session operation.
                IAsyncResult asyncResult = NetworkSession.BeginCreate(
                    this.sessionType,
                    NetworkSessionComponent.MaxLocalGamers,
                    NetworkSessionComponent.MaxGamers,
                    null,
                    null);

                // Activate the network busy screen, which will display
                // an animation until this operation has completed.
                NetworkBusyScreen busyScreen = new NetworkBusyScreen(asyncResult);

                busyScreen.OperationCompleted += this.CreateSessionOperationCompleted;

                ScreenManager.AddScreen(busyScreen);
            }
            catch (NetworkException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
            catch (GamerPrivilegeException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
        }

        /// <summary>
        /// Event handler for when the asynchronous create network session
        /// operation has completed.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event args</param>
        private void CreateSessionOperationCompleted(
            object sender,
            OperationCompletedEventArgs e)
        {
            try
            {
                // End the asynchronous create network session operation.
                NetworkSession networkSession = NetworkSession.EndCreate(e.AsyncResult);

                // Create a component that will manage the session we just created.
                NetworkSessionComponent.Create(ScreenManager, networkSession);

                // Go to the lobby screen.
                ScreenManager.AddScreen(new LobbyScreen(networkSession));
            }
            catch (NetworkException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
            catch (GamerPrivilegeException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
        }

        /// <summary>
        /// Event handler for when the Find Sessions menu entry is selected.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event args</param>
        private void FindSessionsMenuEntrySelected(object sender, EventArgs e)
        {
            try
            {
                // Begin an asynchronous find network sessions operation.
                IAsyncResult asyncResult = NetworkSession.BeginFind(
                    this.sessionType,
                    NetworkSessionComponent.MaxLocalGamers,
                    null,
                    null,
                    null);

                // Activate the network busy screen, which will display
                // an animation until this operation has completed.
                NetworkBusyScreen busyScreen = new NetworkBusyScreen(asyncResult);

                busyScreen.OperationCompleted += this.FindSessionsOperationCompleted;

                ScreenManager.AddScreen(busyScreen);
            }
            catch (NetworkException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
            catch (GamerPrivilegeException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
        }

        /// <summary>
        /// Event handler for when the asynchronous find network sessions
        /// operation has completed.
        /// </summary>
        /// <param name="sender">The sender of this event</param>
        /// <param name="e">Standard event args</param>
        private void FindSessionsOperationCompleted(
            object sender,
            OperationCompletedEventArgs e)
        {
            try
            {
                // End the asynchronous find network sessions operation.
                AvailableNetworkSessionCollection availableSessions =
                                                NetworkSession.EndFind(e.AsyncResult);

                if (availableSessions.Count == 0)
                {
                    // If we didn't find any sessions, display an error.
                    availableSessions.Dispose();

                    ScreenManager.AddScreen(
                            new MessageBoxScreen(Resources.NoSessionsFound, false));
                }
                else
                {
                    // If we did find some sessions, proceed to the JoinSessionScreen.
                    ScreenManager.AddScreen(new JoinSessionScreen(availableSessions));
                }
            }
            catch (NetworkException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
            catch (GamerPrivilegeException exception)
            {
                ScreenManager.AddScreen(new NetworkErrorScreen(exception));
            }
        }

        #endregion
    }
}
