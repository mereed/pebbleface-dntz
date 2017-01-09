module.exports = [
  {
    "type": "heading",
    "defaultValue": "Day & Night Timezones Config"
  },
     {
        "type": "text",
        "defaultValue": "<h6>A white toggle = OFF, an orange toggle = ON</h6>",
      },
  {
    "type": "section",
    "items": [	
 	  {
        "type": "heading",
        "defaultValue": "<h5>Vibration</h5>"
      }
		]
  },
      {
        "type": "toggle",
        "messageKey": "bluetoothvibe",
        "label": "Bluetooth Vibration",
        "defaultValue": false
      },
	  {
        "type": "toggle",
        "messageKey": "hourlyvibe",
        "label": "Vibrate each hour",
        "defaultValue": false
      },

	{
    "type": "section",
    "items": [
 	{
        "type": "heading",
        "defaultValue": "<h5>Display - Select map style</h5>"
	}
  ]
	},
    {
  "type": "radiogroup",
  "messageKey": "mapstyle",
//  "label": "Select map style",
  "options": [
    { 
      "label": "Satellite light Or BW dither 1 (mono)", 
      "value": 0
    },
    { 
      "label": "Satellite medium Or BW dither 2 (mono)", 
      "value": 1
    },
	{ 
      "label": "Satellite dark Or BW dither 3 (mono)", 
      "value": 2
    },
	{ 
      "label": "Natural Veg. w/ grid 1 Or BW outline (mono)", 
      "value": 3
    },
	{ 
      "label": "Natural Veg. w/ grid 2 Or BW grid (mono)", 
      "value": 4
    },
	{ 
      "label": "Simple Colour Or BW solid (mono)", 
      "value": 5
    },
	{ 
      "label": "Black / Grey / White Or BW inverse (mono)", 
      "value": 6
    }
   ]
 },
  {
        "type": "text",
        "defaultValue": "<em>The styles listed above after the 'Or' apply to monochrome (b&w) pebbles.  To view a style after making and saving your selection, you will need to press the watch MENU and then BACK button to redraw the map with your selected style. IT WILL NOT UPDATE ON ITS OWN.</em>",
   },
  {
        "type": "text",
        "defaultValue": "<h6>This watchface will continue to be free.  If you find it useful, please consider making a <a href='https://www.paypal.me/markchopsreed'>small donation here</a>. Thankyou.</h6>",
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];