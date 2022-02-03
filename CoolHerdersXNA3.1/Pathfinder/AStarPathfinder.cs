//-----------------------------------------------------------------------------
// <copyright file="AStarPathfinder.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Pathfinder
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text;
    using CoolHerders.GameItems;
    using CoolHerders.Housekeeping;
    using Microsoft.Xna.Framework;
    using Microsoft.Xna.Framework.GamerServices;
    using Microsoft.Xna.Framework.Net;

    /// <summary>
    /// An implementation of A* pathfinding
    /// </summary>
    internal class AStarPathfinder
    {
        /// <summary>
        /// The pool of maze coordinates that will be used in pathfinding
        /// </summary>
        public static Pool<MazeCoordinate> MazePool = new Pool<MazeCoordinate>();

        /// <summary>
        /// The A* open list
        /// </summary>
        private List<MazeCoordinate> openList;

        /// <summary>
        /// The A* closed list
        /// </summary>
        private List<MazeCoordinate> closedList;

        /// <summary>
        /// The starting point for the routing path
        /// </summary>
        private MazeCoordinate startingPoint;

        /// <summary>
        /// The ending point for which a route is desired
        /// </summary>
        private MazeCoordinate endingPoint;

        /// <summary>
        /// The game component which is managing this pathfinder
        /// </summary>
        private MazeGameComponent parentComponent;

        /// <summary>
        /// Initializes a new instance of the AStarPathfinder class
        /// </summary>
        /// <param name="parentComponent">The game component that this pathfinder is running under</param>
        public AStarPathfinder(MazeGameComponent parentComponent)
        {
            this.parentComponent = parentComponent;
            this.openList = new List<MazeCoordinate>();
            this.closedList = new List<MazeCoordinate>();
        }

        /// <summary>
        /// Computes a path from the starting point to the ending point
        /// </summary>
        /// <param name="startingPoint">The starting point of the path</param>
        /// <param name="endingPoint">The ending point of the path</param>
        /// <returns>A list of maze coordinates to walk to in order to arrive at the destination, or null if no path</returns>
        public List<MazeCoordinate> ComputePath(MazeCoordinate startingPoint, MazeCoordinate endingPoint)
        {
            this.startingPoint = startingPoint;
            this.endingPoint = endingPoint;

            // A* step 1
            this.openList.Add(startingPoint);

            while (this.openList.Count > 0)
            {
                MazeCoordinate currentSquare = null;
                int lowestF = int.MaxValue;

                // A* Step 2a
                foreach (MazeCoordinate tempSquare in this.openList)
                {
                    if (tempSquare.ScoreF < lowestF)
                    {
                        lowestF = tempSquare.ScoreF;
                        currentSquare = tempSquare;
                    }
                }

                if (currentSquare == null)
                {
                    return null;
                }

                // A* Step 2b
                this.openList.Remove(currentSquare);
                this.closedList.Add(currentSquare);

                if ((currentSquare.CellColumn == this.endingPoint.CellColumn) && (currentSquare.CellRow == this.endingPoint.CellRow))
                {
                    // VICTORY!
                    List<MazeCoordinate> resultList = new List<MazeCoordinate>();
                    MazeCoordinate reorderList = currentSquare;
                    while (reorderList != null)
                    {
                        MazeCoordinate reorderCell = reorderList;
                        resultList.Insert(0, reorderCell);
                        reorderList = reorderCell.ParentCell;
                        this.closedList.Remove(reorderCell);
                    }

                    MazeCoordinate extraCell = resultList[0];
                    resultList.RemoveAt(0);
                    MazePool.Release(extraCell);
                    this.ReleaseAllCoordinates();
                    return resultList;
                }

                // A* Step 2c
                this.ConsiderCell(currentSquare, -1, 0);
                this.ConsiderCell(currentSquare, 1, 0);
                this.ConsiderCell(currentSquare, 0, -1);
                this.ConsiderCell(currentSquare, 0, 1);
            }

            this.ReleaseAllCoordinates();
            return null;
        }

        /// <summary>
        /// Cleanup function that returns all coordinates to the pool
        /// </summary>
        private void ReleaseAllCoordinates()
        {
            foreach (MazeCoordinate cell in this.openList)
            {
                MazePool.Release(cell);
            }

            foreach (MazeCoordinate cell in this.closedList)
            {
                MazePool.Release(cell);
            }

            MazePool.Release(this.endingPoint);
        }

        /// <summary>
        /// Considers if a cell is suitable, or best for travel
        /// </summary>
        /// <param name="cell">The cell we are considering</param>
        /// <param name="offsetRow">The row offset we are considering to move</param>
        /// <param name="offsetColumn">The column offset we are considering to move</param>
        private void ConsiderCell(MazeCoordinate cell, int offsetRow, int offsetColumn)
        {
            // If it is impassable, absolutely impossible to pass, impassable
            MazeItem candidateItem = this.parentComponent.GetMazeItem(cell.CellRow + offsetRow, cell.CellColumn + offsetColumn);
            if ((!candidateItem.IsPassable) && (!candidateItem.Destructible))
            {
                return;
            }

            foreach (MazeCoordinate tempCell in this.closedList)
            {
                if ((tempCell.CellColumn == cell.CellColumn + offsetColumn) && (tempCell.CellRow == cell.CellRow + offsetRow))
                {
                    return;
                }
            }

            if (cell.ScoreG <= 20)
            {
                // If we are close to our starting point, consider the positions of any players
                foreach (PlayerCharacter player in this.parentComponent.PlayerCharacters)
                {
                    if (null != player)
                    {
                        if ((this.startingPoint.CellColumn != cell.CellColumn) || (this.startingPoint.CellRow != cell.CellRow))
                        {
                            if ((player.CharacterColumn == cell.CellColumn) && (player.CharacterRow == cell.CellRow))
                            {
                                return;
                            }
                        }
                    }
                }
            }

            // Check the open list
            foreach (MazeCoordinate tempCell in this.openList)
            {
                if ((tempCell.CellColumn == cell.CellColumn + offsetColumn) && (tempCell.CellRow == cell.CellRow + offsetRow))
                {
                    // If it is in the open list
                    int pendingG = cell.ScoreG + 10;
                    if (pendingG < tempCell.ScoreG)
                    {
                        tempCell.ParentCell = cell;
                        tempCell.ScoreG = pendingG;
                        tempCell.ScoreF = tempCell.ScoreG + tempCell.ScoreH;
                    }

                    return;
                }
            }

            // If it is NOT in the open list
            MazeCoordinate newCell = MazePool.Fetch();
            newCell.CellColumn = cell.CellColumn + offsetColumn;
            newCell.CellRow = cell.CellRow + offsetRow;
            newCell.ParentCell = cell;
            newCell.ScoreG = newCell.ParentCell.ScoreG + 10;
            newCell.ScoreH = (Math.Abs(newCell.CellColumn - this.endingPoint.CellColumn) + Math.Abs(newCell.CellRow - this.endingPoint.CellRow)) * 10;
            newCell.ScoreF = newCell.ScoreG + newCell.ScoreH;
            this.openList.Add(newCell);
        }
    }
}