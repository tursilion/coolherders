//-----------------------------------------------------------------------------
// <copyright file="ToyLevel.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.LevelDescriptions
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;
    using Microsoft.Xna.Framework.GamerServices;

    /// <summary>
    /// This is a compiled in level for the Toy Factory
    /// </summary>
    public class ToyLevel : LevelDescription
    {
        /// <summary>
        /// Initializes a new instance of the ToyLevel class
        /// </summary>
        /// <param name="mapNumber">The map number from this level we are working with</param>
        public ToyLevel()
        {
            this.theLevelName = "toy";
            this.thePrintableName = "Toy Factory";
            this.theLevelNumber = 3;
            this.theBackgroundSongName = "ToyFactoryBackground";
            this.sheepAreGhosts = false;
            this.zeusWearsAFro = false;

            this.singlePlayerEnemies = new List<PlayerInformation>();

            PlayerInformation enemyPlayer = new PlayerInformation();
            enemyPlayer.CharacterClass = "NH5";
            enemyPlayer.PlayerActive = true;
            enemyPlayer.SeatNumber = 1;
            enemyPlayer.PlayerColorIndex = 0;
            enemyPlayer.SkillLevel = 3;
            this.SinglePlayerEnemies.Add(enemyPlayer);
        }

        public override bool IsLevelUnlocked
        {
            get
            {
                return LockByHighestLevel();
            }
        }
    }
}
