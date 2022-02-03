//-----------------------------------------------------------------------------
// <copyright file="Pool.cs" company="HarmlessLion">
//  Copyright (C) HarmlessLion. All rights reserved.
// </copyright>
//-----------------------------------------------------------------------------

namespace CoolHerders.Housekeeping
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Text;

    /// <summary>
    /// A persistent object pool, to prevent excessive garbage being created when objects are frequently reused.
    /// </summary>
    /// <typeparam name="T">The type of object that will be placed in this pool</typeparam>
    internal class Pool<T> where T : new()
    {
        /// <summary>
        /// A stack used to keep track of all outstanding pooled objects
        /// </summary>
        private Stack<T> stack;

        /// <summary>
        /// A debugging counter that tracks how many requests for object allocation are outstanding
        /// </summary>
        private int allocationCount = 0;

        /// <summary>
        /// Initializes a new instance of the Pool class
        /// </summary>
        public Pool()
        {
            this.stack = new Stack<T>();
        }

        /// <summary>
        /// Initializes a new instance of the Pool class, given a default size
        /// </summary>
        /// <param name="size">The default size of the pool</param>
        public Pool(int size)
        {
            this.stack = new Stack<T>(size);
            for (int i = 0; i < size; i++)
            {
                this.stack.Push(new T());
            }
        }

        /// <summary>
        /// Gets an object from the pool, or allocates a new object if needed
        /// </summary>
        /// <returns>A new object of type T</returns>
        public T Fetch()
        {
            if (this.stack.Count > 0)
            {
                return this.stack.Pop();
            }
            else
            {
                this.allocationCount++;
                return new T();
            }
        }

        /// <summary>
        /// Releases an object back to the pool
        /// </summary>
        /// <param name="item">An object of type T that was acquired from the pool</param>
        public void Release(T item)
        {
            if (item != null)
            {
                this.stack.Push(item);
            }
        }
    }
}
