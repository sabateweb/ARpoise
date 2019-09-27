/*
UnityARCameraManager.cs - Camera manager of the ARKit based version of image trigger ARpoise, aka AR-vos.

ARPOISE - Augmented Reality Point Of Interest Service 

This file is part of Arpoise. 

This file is derived from image trigger example of the Unity-ARKit-Plugin

https://bitbucket.org/Unity-Technologies/unity-arkit-plugin

The license of this project says:

All contents of this repository 
except for the contents of  the /Assets/UnityARKitPlugin/Examples/FaceTracking/SlothCharacter folder and its subfolders 
are released under the MIT License, which is listed under /LICENSES/MIT_LICENSE file.

The MIT License (MIT)

Copyright (c) 2017, Unity Technologies

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
For more information on 

Tamiko Thiel, see www.TamikoThiel.com/
Peter Graf, see www.mission-base.com/peter/
Arpoise, see www.Arpoise.com/

*/
using com.arpoise.arpoiseapp;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR.iOS;

public class UnityARCameraManager : ArBehaviourSlam
{
#if IS_SLAM_APP
#else
    public GameObject AnchorManager;
#endif

    public Camera m_camera;
    private UnityARSessionNativeInterface m_session;
    private Material savedClearMaterial;

    [Header("AR Config Options")]
    public UnityARAlignment startAlignment = UnityARAlignment.UnityARAlignmentGravity;
    public UnityARPlaneDetection planeDetection = UnityARPlaneDetection.Horizontal;
    public bool getPointCloud = true;
    public bool enableLightEstimation = true;
    public bool enableAutoFocus = true;
    public UnityAREnvironmentTexturing environmentTexturing = UnityAREnvironmentTexturing.UnityAREnvironmentTexturingNone;

    [Header("Image Tracking")]
    public ARReferenceImagesSet detectionImages = null;
    public int maximumNumberOfTrackedImages = 0;

    [Header("Object Tracking")]
    public ARReferenceObjectsSetAsset detectionObjects = null;
    private bool sessionStarted = false;

    public ARKitWorldTrackingSessionConfiguration sessionConfiguration
    {
        get
        {
            ARKitWorldTrackingSessionConfiguration config = new ARKitWorldTrackingSessionConfiguration();
            config.planeDetection = planeDetection;
            config.alignment = startAlignment;
            config.getPointCloudData = getPointCloud;
            config.enableLightEstimation = enableLightEstimation;
            config.enableAutoFocus = enableAutoFocus;
            config.maximumNumberOfTrackedImages = maximumNumberOfTrackedImages;
            config.environmentTexturing = environmentTexturing;
            if (detectionImages != null)
            {
                config.referenceImagesGroupName = detectionImages.resourceGroupName;
            }

            if (detectionObjects != null)
            {
                config.referenceObjectsGroupName = "";  //lets not read from XCode asset catalog right now
                config.dynamicReferenceObjectsPtr = m_session.CreateNativeReferenceObjectsSet(detectionObjects.LoadReferenceObjectsInSet());
            }

            return config;
        }
    }

    // Use this for initialization
    protected override void Start()
    {
        base.Start();

        m_session = UnityARSessionNativeInterface.GetARSessionNativeInterface();

        Application.targetFrameRate = 60;

#if IS_SLAM_APP
        StartArSession();
#else
#endif

        if (m_camera == null)
        {
            m_camera = Camera.main;
        }
    }

    protected void StartArSession()
    {
        var config = sessionConfiguration;
        if (config.IsSupported)
        {
            m_session.RunWithConfig(config);
            UnityARSessionNativeInterface.ARFrameUpdatedEvent += FirstFrameUpdate;
        }
    }

    protected void StartArSession(Texture2D texture)
    {
        if (texture != null)
        {
            var config = sessionConfiguration;
            if (config.IsSupported)
            {
                byte[] bytes = texture.EncodeToJPG();
                m_session.RunWithConfigAndImage(config, bytes.Length, bytes, 0.25f);
                UnityARSessionNativeInterface.ARFrameUpdatedEvent += FirstFrameUpdate;
            }
        }
    }

    protected void StartArSession(Dictionary<int, TriggerObject> triggerObjects)
    {
        var config = sessionConfiguration;
        if (config.IsSupported)
        {
            foreach (var key in triggerObjects.Keys)
            {
                var triggerObject = triggerObjects[key];
                byte[] bytes = triggerObject.texture.EncodeToJPG();
                m_session.StoreTriggerImage("" + key, bytes.Length, bytes, triggerObject.width);
            }
            m_session.RunWithConfigAndImages(config);
            UnityARSessionNativeInterface.ARFrameUpdatedEvent += FirstFrameUpdate;
        }
    }

    private void OnDestroy()
    {
        m_session.Pause();
    }

    private void FirstFrameUpdate(UnityARCamera cam)
    {
        sessionStarted = true;
        UnityARSessionNativeInterface.ARFrameUpdatedEvent -= FirstFrameUpdate;
    }

    public void SetCamera(Camera newCamera)
    {
        if (m_camera != null)
        {
            UnityARVideo oldARVideo = m_camera.gameObject.GetComponent<UnityARVideo>();
            if (oldARVideo != null)
            {
                savedClearMaterial = oldARVideo.m_ClearMaterial;
                Destroy(oldARVideo);
            }
        }
        SetupNewCamera(newCamera);
    }

    private void SetupNewCamera(Camera newCamera)
    {
        m_camera = newCamera;

        if (m_camera != null)
        {
            UnityARVideo unityARVideo = m_camera.gameObject.GetComponent<UnityARVideo>();
            if (unityARVideo != null)
            {
                savedClearMaterial = unityARVideo.m_ClearMaterial;
                Destroy(unityARVideo);
            }
            unityARVideo = m_camera.gameObject.AddComponent<UnityARVideo>();
            unityARVideo.m_ClearMaterial = savedClearMaterial;
        }
    }

    protected override void Update()
    {
        base.Update();
#if IS_SLAM_APP
#else
        if (!sessionStarted)
        {
            if (!HasTriggerImages)
            {
                return;
            }
            var anchorManager = AnchorManager.GetComponent<ArKitAnchorManager>();
            anchorManager.ArBehaviour = this;
            anchorManager.TriggerObjects = TriggerObjects;
            anchorManager.FitToScanOverlay = FitToScanOverlay;
            StartArSession(anchorManager.TriggerObjects);
        }
#endif
        if (m_camera != null && sessionStarted)
        {
            // JUST WORKS!
            Matrix4x4 matrix = m_session.GetCameraPose();
            m_camera.transform.localPosition = UnityARMatrixOps.GetPosition(matrix);
            m_camera.transform.localRotation = UnityARMatrixOps.GetRotation(matrix);

            m_camera.projectionMatrix = m_session.GetCameraProjection();
        }
    }
}