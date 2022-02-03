//-----------------------------------------------------------------------------
// <copyright file="ICollidable.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders
{
    using System;
    using Microsoft.Xna.Framework;

    /// <summary>
    /// An interface to which an object must conform if it is to be passed into the generic collision system
    /// </summary>
    internal interface ICollidable
    {
        /// <summary>
        /// Gets or sets a value indicating whether collisions should be checked
        /// </summary>
        bool CheckCollisions
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the bounding box for collision purposes
        /// </summary>
        BoundingBox CollisionBox
        {
            get;
            set;
        }
    }
}
