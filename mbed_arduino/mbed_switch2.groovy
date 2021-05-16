/**
 *  Mbed Switch
 *
 *  Copyright 2016 Cameron Reid
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 */

 private getMODEL_MAP() {
    [
        'Mbed Switch 2' : 2
    ]
}

metadata {
	definition (name: "Mbed Switch 2", namespace: "CR76", author: "Cameron Reid") {
    capability "Refresh"
    capability "Polling"
    capability "Switch"

   	fingerprint endpointId: "76", profileId: "0104", inClusters: "0000,0006", model: "Mbed Switch 2"
	}


	simulator {
		// TODO: define status and reply messages here
	}

	tiles(scale:2) {
		multiAttributeTile(name:"Switch1", type:"generic", width:6, height:4) {
    		tileAttribute("device.switch1", key: "PRIMARY_CONTROL") {
        	attributeState "on", label:'${name}', action: "Switch1.off", backgroundColor:"#79b821"
        	attributeState "off", label:'${name}', action: "Switch1.on", backgroundColor:"#ffffff"
            }
        }
        standardTile("refresh", "device.refresh", inactiveLabel: false, decoration: "flat", width:2, height:2 ) {
			state "default", action:"refresh.refresh", icon:"st.secondary.refresh"
		}
    multiAttributeTile(name:"Switch2", type:"generic", width:6, height:4) {
        tileAttribute("device.switch2", key: "PRIMARY_CONTROL") {
          attributeState "on", label:'${name}', action: "Switch2.off", backgroundColor:"#79b821"
          attributeState "off", label:'${name}', action: "Switch2.on", backgroundColor:"#ffffff"
            }
        }
        standardTile("refresh", "device.refresh", inactiveLabel: false, decoration: "flat", width:2, height:2 ) {
      state "default", action:"refresh.refresh", icon:"st.secondary.refresh"
    }
		main (["Switch1"])
		details (["Switch1", "refresh"])
	}
}

// parse events into attributes
def parse(String description) {
	log.debug "Parsing '${description}'"
    def event = zigbee.getEvent(description)
    if (event) {
        sendEvent(event)
        log.debug "Event '${event}'"
    }
    else {
        log.warn "DID NOT PARSE MESSAGE for description : $description"
        log.debug zigbee.parseDescriptionAsMap(description)
    }

}

def on() {
	log.debug "LED 1 on()"
    zigbee.on()
}

def off() {
	log.debug "LED 1 off()"
    zigbee.off()
}

def poll(){
	log.debug "Poll is calling refresh"
	refresh()
}

def refresh() {
	log.debug "sending refresh command"
    def cmd = []
    cmd << "st rattr 0x${device.deviceNetworkId} 0x76 0x0006 0x0000"	// Read on / off attribute at End point 0x76
}
