//-----------------------------------------------------------------------------
// <copyright file="IMessageDisplay.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    #region Using Statements
    using Microsoft.Xna.Framework;
    #endregion

    /// <summary>
    /// Interface used to display notification messages when interesting events occur,
    /// for instance when gamers join or leave the network session. This interface
    /// is registered as a service, so any piece of code wanting to display a message
    /// can look it up from Game.Services, without needing to worry about how the
    /// message display is implemented. In this sample, the MessageDisplayComponent
    /// class implement this IMessageDisplay service.
    /// </summary>
    public interface IMessageDisplay : IDrawable, IUpdateable
    {
        /// <summary>
        /// This function shows a message on the screen
        /// </summary>
        /// <param name="message">The message to display</param>
        /// <param name="parameters">Any parameters required to display the message</param>
        void ShowMessage(string message, params object[] parameters);
    }
}
