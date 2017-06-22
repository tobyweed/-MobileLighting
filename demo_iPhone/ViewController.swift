//
//  ViewController.swift
//  demo_iPhone
//
//  Created by Nicholas Mosier on 5/26/17.
//  Copyright © 2017 Nicholas Mosier. All rights reserved.
//

import UIKit
import CocoaAsyncSocket
import AVFoundation
import Photos

class ViewController: UIViewController {
    var cameraService: CameraService!
    @IBOutlet var videoPreviewView: VideoPreviewView!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.cameraService = CameraService()
        self.videoPreviewView.session = self.cameraService.cameraController.captureSession
        
        cameraService.startBroadcast()
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
    
}

