package main

// #include "app.h"
// #cgo LDFLAGS: -framework Carbon -framework CoreFoundation
import "C"

import (
	"github.com/getlantern/systray"
)

func main() {
	C.start_sketch_deamon()
	systray.Run(onReady, nil)
}

func onReady() {
	systray.SetTitle("✎")
	mQuit := systray.AddMenuItem("Quit", "Quit the whole app")
	go func() {
		for {
			select {
			case <-mQuit.ClickedCh:
				C.stop_sketch_deamon()
				systray.Quit()
			}
		}
	}()

}
