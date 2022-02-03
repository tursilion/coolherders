//-----------------------------------------------------------------------------
// <copyright file="SpeedPickup.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.GameItems
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;

    /// <summary>
    /// An item located in the maze which increases the character's speed when picked up
    /// </summary>
    internal class SpeedPickup : PickupItem
    {
        /// <summary>
        /// Initializes a new instance of the SpeedPickup class
        /// </summary>
        /// <param name="parentComponent">The game component that is managing this pickup</param>
        /// <param name="networkGamer">The gamer that owns this pickup for networking porpoises</param>
        /// <param name="rowCounter">The row where this pickup will appear</param>
        /// <param name="columnCounter">The column where this pickup will appear</param>
        public SpeedPickup(MazeGameComponent parentComponent, NetworkGamer gamerInfo, int rowCounter, int columnCounter)
            : base(parentComponent, gamerInfo, 0, rowCounter, columnCounter)
        {
        }

        /// <summary>
        /// Called when a player touches this speed powerup
        /// </summary>
        /// <param name="acquiringCharacter">The character that touched this powerup</param>
        public override void OnPickup(MovingCharacter acquiringCharacter)
        {
            PlayerCharacter playerChar = (PlayerCharacter)acquiringCharacter;
            if (playerChar.SpeedPowerupAmount < 100.0f)
            {
                playerChar.SpeedPowerupAmount += 25.0f;
                playerChar.SetCharacterSpeed();
            }
        }
    }
}
