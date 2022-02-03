//-----------------------------------------------------------------------------
// <copyright file="ZapperPickup.cs" company="HarmlessLion">
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
    /// An item located in the maze which increaces the player's zapper power when picked up
    /// </summary>
    internal class ZapperPickup : PickupItem
    {
        /// <summary>
        /// Initializes a new instance of the ZapperPickup class
        /// </summary>
        /// <param name="parentComponent">The game component that is managing this pickup</param>
        /// <param name="networkGamer">The gamer that owns this pickup for networking porpoises</param>
        /// <param name="rowCounter">The row where this pickup will appear</param>
        /// <param name="columnCounter">The column where this pickup will appear</param>
        public ZapperPickup(MazeGameComponent parentComponent, NetworkGamer gamerInfo, int rowCounter, int columnCounter)
            : base(parentComponent, gamerInfo, 1, rowCounter, columnCounter)
        {
        }

        /// <summary>
        /// Called when a player touches this zapper pickup
        /// </summary>
        /// <param name="acquiringCharacter">The player that touched the powerup</param>
        public override void OnPickup(MovingCharacter acquiringCharacter)
        {
            PlayerCharacter playerChar = (PlayerCharacter)acquiringCharacter;
            if (playerChar.PlayerZapper.MaxZapperPower < 400)
            {
                playerChar.PlayerZapper.MaxZapperPower = playerChar.PlayerZapper.MaxZapperPower + 100;
            }
        }
    }
}
