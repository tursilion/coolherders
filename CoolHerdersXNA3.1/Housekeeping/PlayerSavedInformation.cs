using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Xna.Framework.Storage;

namespace CoolHerders.Housekeeping
{
    [Serializable]
    public struct PlayerSavedInformation
    {
        /// <summary>
        /// The last level that the player has completed in story mode
        /// This can be 0 if the player is not in the middle of a game
        /// This value will reset when a player starts a new game
        /// </summary>
        public int lastCompletedLevel;

        /// <summary>
        /// The highest level that the player has EVER completed in story mode
        /// This will be 0 when the game is run for the first time
        /// This will eventually be 7, indicating that all levels are unlocked
        /// </summary>
        public int highestCompletedLevel;
    }
}