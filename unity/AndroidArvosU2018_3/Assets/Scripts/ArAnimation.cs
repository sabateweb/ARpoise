﻿/*
ArAnimation.cs - Handling porpoise level animations for Arpoise.

Copyright (C) 2018, Tamiko Thiel and Peter Graf - All Rights Reserved

ARPOISE - Augmented Reality Point Of Interest Service 

This file is part of Arpoise.

    Arpoise is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Arpoise is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Arpoise.  If not, see <https://www.gnu.org/licenses/>.

For more information on 

Tamiko Thiel, see www.TamikoThiel.com/
Peter Graf, see www.mission-base.com/peter/
Arpoise, see www.Arpoise.com/

*/

using System;
using UnityEngine;

namespace com.arpoise.arpoiseapp
{
    public class ArAnimation
    {
        public const string Rotate = "rotate";
        public const string Scale = "scale";
        public const string Transform = "transform";
        public const string Linear = "linear";
        public const string Cyclic = "cyclic";
        public const string Sine = "sine";
        public const string Halfsine = "halfsine";

        public readonly long PoiId;
        public readonly GameObject Wrapper;
        public readonly GameObject GameObject;
        public readonly string Name;
        public readonly string FollowedBy;

        private readonly long _lengthTicks;
        private readonly long _delayTicks;
        private readonly string _type = Transform;
        private readonly string _interpolation = Linear;
        private readonly string _interpolationType = string.Empty;
        private readonly bool _persisting;
        private readonly bool _repeating;
        private readonly float _from;
        private readonly float _to;
        private readonly Vector3 _axis;

        private long _startTicks = 0;

        public ArAnimation(long poiId, GameObject wrapper, GameObject gameObject, PoiAnimation poiAnimation, bool isActive)
        {
            PoiId = poiId;
            Wrapper = wrapper;
            GameObject = gameObject;
            IsActive = isActive;
            if (poiAnimation != null)
            {
                Name = poiAnimation.name;
                _lengthTicks = (long)(10000000.0 * poiAnimation.length);
                _delayTicks = (long)(10000000.0 * poiAnimation.delay);
                if (poiAnimation.type != null)
                {
                    _type = poiAnimation.type.ToLower().Contains(Rotate) ? Rotate
                        : poiAnimation.type.ToLower().Contains(Scale) ? Scale
                        : Transform;
                }
                if (poiAnimation.interpolation != null)
                {
                    _interpolation = poiAnimation.interpolation.ToLower().Contains(Cyclic) ? Cyclic : Linear;
                    _interpolationType = poiAnimation.interpolation.ToLower().Contains(Halfsine) ? Halfsine
                        : poiAnimation.interpolation.ToLower().Contains(Sine) ? Sine
                        : string.Empty;
                }
                _persisting = poiAnimation.persist;
                _repeating = poiAnimation.repeat;
                _from = poiAnimation.from;
                _to = poiAnimation.to;
                _axis = poiAnimation.axis == null ? Vector3.zero
                    : new Vector3(poiAnimation.axis.x, poiAnimation.axis.y, poiAnimation.axis.z);
                FollowedBy = poiAnimation.followedBy;
            }
        }

        public bool IsActive { get; private set; }
        public bool JustActivated { get; private set; }
        public bool JustStopped { get; private set; }

        public void Activate(long worldStartTicks, long nowTicks)
        {
            IsActive = true;
            _startTicks = 0;
            Animate(worldStartTicks, nowTicks);
        }

        public void Animate(long worldStartTicks, long nowTicks)
        {
            JustActivated = false;
            JustStopped = false;

            if (worldStartTicks <= 0 || !IsActive || _lengthTicks < 1 || _delayTicks < 0)
            {
                return;
            }

            if (_delayTicks > 0 && worldStartTicks + _delayTicks > nowTicks)
            {
                return;
            }

            float animationValue = 0;

            if (_startTicks == 0)
            {
                _startTicks = nowTicks;
                JustActivated = true;
            }
            else
            {
                var endTicks = _startTicks + _lengthTicks;
                if (endTicks < nowTicks)
                {
                    if (!_repeating)
                    {
                        Stop(worldStartTicks, nowTicks, false);
                        return;
                    }

                    if (endTicks + _lengthTicks < nowTicks)
                    {
                        _startTicks = nowTicks;
                    }
                    else
                    {
                        _startTicks += _lengthTicks;
                    }
                    JustActivated = true;
                }
                animationValue = (nowTicks - _startTicks) / ((float)_lengthTicks);
            }

            var from = _from;
            var to = _to;

            if (Cyclic.Equals(_interpolation))
            {
                if (animationValue >= .5)
                {
                    animationValue -= .5f;
                    var temp = from;
                    from = to;
                    to = temp;
                }
                animationValue *= 2;
            }

            if (Halfsine.Equals(_interpolationType))
            {
                animationValue = (float)Math.Sin(Math.PI * animationValue);
            }
            else if (Sine.Equals(_interpolationType))
            {
                animationValue = (-1f + (float)Math.Cos(2 * Math.PI * animationValue)) / 2;
            }

            if (animationValue < 0)
            {
                animationValue = -animationValue;
            }

            var animationFactor = from + (to - from) * animationValue;

            if (Rotate.Equals(_type))
            {
                Wrapper.transform.localEulerAngles = new Vector3(
                    _axis.x * animationFactor,
                    _axis.y * animationFactor,
                    _axis.z * animationFactor
                    );
            }
            else if (Scale.Equals(_type))
            {
                Wrapper.transform.localScale = new Vector3(
                    _axis.x == 0 ? 1 : _axis.x * animationFactor,
                    _axis.y == 0 ? 1 : _axis.y * animationFactor,
                    _axis.z == 0 ? 1 : _axis.z * animationFactor
                    );
            }
            else if (Transform.Equals(_type))
            {
                Wrapper.transform.localPosition = new Vector3(
                    _axis.x * animationFactor,
                    _axis.y * animationFactor,
                    _axis.z * animationFactor
                    );
            }
        }

        public void Stop(long worldStartTicks, long nowTicks, bool animate = true)
        {
            if (animate)
            {
                Animate(worldStartTicks, nowTicks);
            }
            JustStopped = true;
            IsActive = false;

            if (!_persisting)
            {
                if (Rotate.Equals(_type))
                {
                    Wrapper.transform.localEulerAngles = Vector3.zero;
                }
                else if (Scale.Equals(_type))
                {
                    Wrapper.transform.localScale = Vector3.one;
                }
                else if (Transform.Equals(_type))
                {
                    Wrapper.transform.localPosition = Vector3.zero;
                }
            }
        }
    }
}