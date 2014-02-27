// Copyright 2013 Google Inc. All rights reserved.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

var settings = window['settings'];

if (settings && settings['region']) {
  for (var i = 0; i < region.options.length; ++i) {
    if (region.options[i].value == settings['region']) {
      region.selectedIndex = i;
    }
  }
}

function save() {
  var msg = {
    'type': 'setsettings',
    'data': {
      'region': region.options[region.selectedIndex].value || 'WW'
    }
  };
  window['opener'].postMessage(msg, '*');
  exit();
}

function exit() {
  chrome.app.window.current().close();
}

cancel.addEventListener('click', exit);
ok.addEventListener('click', save);
