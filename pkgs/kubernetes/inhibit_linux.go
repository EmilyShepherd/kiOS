//go:build linux
// +build linux

/*
Copyright 2021 The Kubernetes Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

		http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

package systemd

import (
	"time"

	"github.com/fsnotify/fsnotify"
	"k8s.io/klog/v2"
)

type Inhibitor struct {}

func NewDBusCon() (*Inhibitor, error) {
	return &Inhibitor{}, nil
}

type InhibitLock uint16

const (
	// The directory and file to look for shutdown events.
	systemRunDir			 = "/run/system"
	systemShutdownFile = systemRunDir + "/shutdown"
)

// The NodeShutdownManager checks the current system setting for grace.
// As we do not actually have a way of setting this (and will just wait
// for kubelet no matter what) let's just set an absurdly high figure to
// keep the kubelet happy.
func (bus *Inhibitor) CurrentInhibitDelay() (time.Duration, error) {
	duration := time.Duration(1000) * time.Second
	return duration, nil
}

// Under systemd, the NodeShutdownManager needs to "request" an inhibit
// lock. As this shim was built specifically for KiOS, we don't need to
// bother with any of the overhead of actually checking anything.
//
// This will always return an "InhibitLock" in line with the method
// signature, but this is effectively worthless and is not checked again
// anywhere.
func (bus *Inhibitor) InhibitShutdown() (InhibitLock, error) {
	return InhibitLock(1), nil
}

// Called by the NodeShutdownManager when it has finished killing all
// pods and is ready for the system to continue shutting down.
//
// TODO: Call the init process from here to actually shut the system
// down.
// (NB: Looks like this is called also when refreshing the lock, so make
// sure you do NOT kick off a shutdown without checking one is in
// progress first!
func (bus *Inhibitor) ReleaseInhibitLock(lock InhibitLock) error {
	return nil
}

// This watches the /run/system directory for a "shutdown" file. If it
// created, this will trigger kubelet's Graceful Node Shutdown behaviour
// and kill all the pods in a graceful way.
// NB: Deleting the file will trigger kubelet to cancel the shutdown.
func (bus *Inhibitor) MonitorShutdown() (<-chan bool, error) {
	shutdownChan := make(chan bool, 1)
	watcher, err := fsnotify.NewWatcher()
	if err != nil {
		return nil, err
	}
	err = watcher.Add(systemRunDir)

	go func() {
		for {
			select {
			case event, ok := <-watcher.Events:
				if !ok {
					close(shutdownChan)
					return
				}
				if event.Name == systemShutdownFile {
					if event.Op&fsnotify.Create == fsnotify.Create {
						shutdownChan <- true
					} else if event.Op&fsnotify.Remove == fsnotify.Remove {
						shutdownChan <- false
					}
				}
			case err, ok := <-watcher.Errors:
				if !ok {
					close(shutdownChan)
					return
				}
				close(shutdownChan)
				klog.ErrorS(err, "Watcher error")
			}
		}
	}()

	return shutdownChan, nil
}

// Called as part of the NodeShutdownManager's init system if it
// believes that the current system shutdown grace period is too low.
//
// No-op for the moment as we currently just give the kubelet as long as
// it needs.
func (bus *Inhibitor) OverrideInhibitDelay(inhibitDelayMax time.Duration) error {
	return nil
}

// Stub for the NodeShutdownManager to call
//
// It seems odd that this method is exported - seems like it should be a
// private method called by OverrideInhibitDelay so that the manager
// does not need to be aware of the implementation differences between
// override and reload.
func (bus *Inhibitor) ReloadLogindConf() error {
	return nil
}

