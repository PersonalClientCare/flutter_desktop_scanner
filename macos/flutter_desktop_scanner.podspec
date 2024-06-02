#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint flutter_desktop_scanner.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'flutter_desktop_scanner'
  s.version          = '1.0.0'
  s.summary          = 'A desktop scanner plugin'
  s.description      = <<-DESC
  Making scanning with physical scanners on desktop a breeze!
                       DESC
  s.homepage         = 'https://github.com/PersonalClientCare/flutter_desktop_scanner'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'PersonalClientCare' => 'habereder.korbinian@personalclientcare.com' }

  s.source           = { :path => '.' }
  s.source_files     = 'Classes/**/*'
  s.dependency 'FlutterMacOS'

  s.platform = :osx, '10.11'
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
  s.swift_version = '5.0'
end
