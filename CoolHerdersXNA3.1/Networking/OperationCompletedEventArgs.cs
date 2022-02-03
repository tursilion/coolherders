//-----------------------------------------------------------------------------
// <copyright file="OperationCompletedEventArgs.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
//  Copyright (C) Microsoft.  From XNA sample code.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    #region Using Statements
    using System;
    #endregion

    /// <summary>
    /// Custom EventArgs class used by the NetworkBusyScreen.OperationCompleted event.
    /// </summary>
    internal class OperationCompletedEventArgs : EventArgs
    {
        /// <summary>
        /// The result of any network operations that we would generate an event for
        /// </summary>
        private IAsyncResult asyncResult;

        #region Initialization

        /// <summary>
        /// Initializes a new instance of the OperationCompletedEventArgs class
        /// </summary>
        /// <param name="asyncResult">The async result from the network operation</param>
        public OperationCompletedEventArgs(IAsyncResult asyncResult)
        {
            this.asyncResult = asyncResult;
        }

        #endregion

        #region Properties

        /// <summary>
        /// Gets the IAsyncResult associated with
        /// the network operation that has just completed.
        /// </summary>
        public IAsyncResult AsyncResult
        {
            get { return this.asyncResult; }
        }

        #endregion
    }
}
