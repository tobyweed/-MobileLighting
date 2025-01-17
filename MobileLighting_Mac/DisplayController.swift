//
// DisplayController.swift
// MobileLighting_Mac
//
// Contains the DisplayController class, which is used to manage the displays
//

import Cocoa
import CoreGraphics
import SwitcherCtrl

class DisplayController: NSWindowController {
    //MARK: Properties
    var windows = [FullscreenWindow]()  // windows currently being displayed
    var currentWindow: FullscreenWindow? {
        get {
            return windows.first
        }
    }
    var switcher: Switcher?
    
    // createCGImage(filePath:)
    //  -filePath: file path (String)
    //  -returns CGImage object, which can directly be drawn by CGContexts (from Quartz 2D graphics library)
    func createCGImage(filePath: String) -> CGImage {
        let url = NSURL(fileURLWithPath: filePath)
        let dataProvider = CGDataProvider(url: url)
        return CGImage(jpegDataProviderSource: dataProvider!, decode: nil, shouldInterpolate: false, intent: .defaultIntent)!
    }
    
    
    //MARK: Functions
    
    func turnOn(projector: Int) {
        if let switcher = switcher {
            switcher.turnOn(projector)
        }
    }
    
    func turnOn(projectors: [Int]) {
        for projector in projectors {
            turnOn(projector: projector)
        }
    }
    
    func turnOnAllProjectors() {
        for projector in 0..<8 {
            turnOn(projector: projector)
        }
    }
    
    func turnOff(projector: Int) {
        if let switcher = switcher {
            switcher.turnOff(projector)
        }
    }
    
    func turnOff(projectors: [Int]) {
        for projector in projectors {
            turnOff(projector: projector)
        }
    }
    
    func turnOffAllProjectors() {
        for projector in 0..<8 {
            turnOff(projector: projector)
        }
    }
    
    // createNewWindow(on:)
    //  -on: NSScreen -> screen to create new window on
    //  creates & displays new window and adds to list of FullscreenWindows
    func createNewWindow(on screen: NSScreen) {
        let newWindow = FullscreenWindow(on: screen)
        windows.append(newWindow)
    }
    
    func configureDisplaySettings(horizontal: Bool = false, inverted: Bool = false, screenID: Int? = nil) {
        let window: FullscreenWindow
        if let screenID = screenID {
            window = windows[screenID]
        } else {
            window = currentWindow!
        }
        window.configureDisplaySettings(horizontal: horizontal, inverted: inverted)
    }
    
    func displayBinaryCode(forBit bit: Int, system: BinaryCodeSystem) {
        guard let currentWindow = currentWindow else {
            print("DisplayController: could not display binary code; no current window set.")
            return
        }
        currentWindow.displayBinaryCode(forBit: bit, system: system)
    }
}
