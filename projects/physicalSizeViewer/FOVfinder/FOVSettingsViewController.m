//
//  FOVSettingsViewController.m
//  FOVfinder
//
//  Created by Tim Gleue on 25.03.14.
//  Copyright (c) 2015 Tim Gleue ( http://gleue-interactive.com )
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#import "FOVSettingsViewController.h"
#import "FOVFormatViewController.h"
#import "FOVGravityViewController.h"
#import "bodypart.h"



@interface FOVSettingsViewController () {
    
    NSInteger _currentRow;
}

@property (weak, nonatomic) IBOutlet UITableViewCell *formatCell;
@property (weak, nonatomic) IBOutlet UITableViewCell *gravityCell;
@property (weak, nonatomic) IBOutlet UITableViewCell *distanceCell;
@property (weak, nonatomic) IBOutlet UITableViewCell *heightCell;
@property (weak, nonatomic) IBOutlet UITableViewCell *sizeCell;
@property (weak, nonatomic) IBOutlet UITableViewCell *bodyPartCell;


@property (weak, nonatomic) IBOutlet UILabel *lensAdjustLabel;
@property (weak, nonatomic) IBOutlet UISwitch *lensAdjustSwitch;
@property (weak, nonatomic) IBOutlet UIStepper *lensAdjustStepper;

@property (weak, nonatomic) UIPickerView *distancePicker;
@property (weak, nonatomic) UIPickerView *heightPicker;
@property (weak, nonatomic) UIPickerView *sizePicker;
@property (weak, nonatomic) UIPickerView *bodyPartPicker;


@end

@implementation FOVSettingsViewController
@synthesize bodyParts;


- (void) loadView {
     [super loadView];
    
    BodyPart *fingerTip = [[BodyPart alloc] init];
    fingerTip.title = @"FingerTip";
    fingerTip.distance = 10.0;
    fingerTip.depth = -3.0;
    fingerTip.size = CGSizeMake(3.0,3.0);
    fingerTip.idx = 0;
    
    BodyPart *hand = [[BodyPart alloc] init];
    hand.title = @"Hand";
    hand.depth = -7.0;
    hand.distance = 12.0;
    hand.size = CGSizeMake(7.0,10.0);
    hand.idx = 1;
    
    BodyPart *back = [[BodyPart alloc] init];
    back.title = @"Back";
    back.depth = -15.0;
    back.distance = 25.0;
    back.size = CGSizeMake(15.0,20.0);
    back.idx = 2;
    
    self.bodyParts = @[fingerTip, hand, back];
    

}


- (void)dealloc {
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewDidLoad {
    
    [super viewDidLoad];
    
    _currentRow = -1;
    NSLog(@"%lu Body Parts ", (unsigned long)[self.bodyParts count]);
}

- (void)viewWillAppear:(BOOL)animated {
    
    [super viewWillAppear:animated];
    
    [self updateCells];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(formatChanged:) name:FOVFormatViewControllerFormatChanged object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(gravityChanged:) name:FOVGravityViewControllerGravityChanged object:nil];
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
    
    if ([segue.identifier isEqualToString:@"OpenFormatSelection"]) {
        
        FOVFormatViewController *controller = segue.destinationViewController;
        
        controller.videoFormat = self.videoFormat;
        
    } else if ([segue.identifier isEqualToString:@"OpenGravitySelection"]) {
        
        FOVGravityViewController *controller = segue.destinationViewController;
        
        controller.videoGravity = self.videoGravity;
    }
}

#pragma mark - Actions

- (IBAction)toggleLensAdjustment:(id)sender {
    
    self.lensAdjustmentEnabled = self.lensAdjustSwitch.on;
    
    [self updateLensCells];
}

- (IBAction)lensAdjustmentChanged:(id)sender {
    
    self.lensAdjustmentFactor = self.lensAdjustStepper.value / 100.0;
    
    [self updateLensCells];
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    
  
    
    if (indexPath.section == 1 && indexPath.row != _currentRow) {
        
        _currentRow = indexPath.row;
        
        UIPickerView *picker = [[UIPickerView alloc] init];
        
        picker.dataSource = self;
        picker.delegate = self;
        
        NSInteger row = 0;
        
        switch (_currentRow) {
           
            case 0:
                
                self.bodyPartPicker = picker;
                row = [self pickerView:picker rowForValue:@(self.overlayBodyPart.idx)];
                break;
                
            case 1:
                
                self.distancePicker = picker;
                row = [self pickerView:picker rowForValue:@(self.overlayDistance)];
                break;
                
            case 2:
                
                self.heightPicker = picker;
                row = [self pickerView:picker rowForValue:@(self.overlayHeight)];
                break;
                
            case 3:
                
                self.sizePicker = picker;
                row = [self pickerView:picker rowForValue:[NSValue valueWithCGSize:self.overlaySize]];
                break;
                
            default:
                break;
        }
        
        [picker selectRow:row inComponent:0 animated:NO];
        
        UITextField *field = [[UITextField alloc] initWithFrame:CGRectZero];
        
        field.inputView = picker;
        field.delegate = self;
        
        UITableViewCell *cell = [tableView cellForRowAtIndexPath:indexPath];
        
        [cell insertSubview:field belowSubview:cell.contentView];
        
        [field becomeFirstResponder];
    }
}

#pragma mark - TextField delegate

- (void)textFieldDidEndEditing:(UITextField *)textField {
    
    [textField removeFromSuperview];
}

#pragma mark - Picker data source

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
    
    return 1;
}




static CGFloat distances[] = { 10.0, 20.0, 30.0, 40.0};
static CGFloat heights[] = { -10.0, -30.0};
static CGSize sizes[] = { { 10.0, 10.0 }, { 15.0, 20.0 } };

- (NSInteger)pickerView:(UIPickerView *)pickerView rowForValue:(id)value {
 
    if (pickerView == self.bodyPartPicker) {
        
        int bid = [(NSNumber *)value intValue];
        for (NSInteger idx = 0; idx < [self.bodyParts count]; idx++) {
            
            if ([[self.bodyParts objectAtIndex:idx] idx] == bid) return idx;
        }
        
    } else if (pickerView == self.distancePicker) {
        
        CGFloat distance = [(NSNumber *)value doubleValue];
        
        for (NSInteger idx = 0; idx < sizeof(distances) / sizeof(distances[0]); idx++) {
            
            if (distances[idx] == distance) return idx;
        }
        
    } else if (pickerView == self.heightPicker) {
        
        CGFloat height = [(NSNumber *)value doubleValue];
        
        for (NSInteger idx = 0; idx < sizeof(heights) / sizeof(heights[0]); idx++) {
            
            if (heights[idx] == height) return idx;
        }
        
    } else if (pickerView == self.sizePicker) {
        
        CGSize size = [(NSValue *)value CGSizeValue];
        
        for (NSInteger idx = 0; idx < sizeof(sizes) / sizeof(sizes[0]); idx++) {
            
            if (CGSizeEqualToSize(sizes[idx], size)) return idx;
        }
    }
    
    return 0;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {
    
    if (pickerView == self.bodyPartPicker) {
        
        return [self.bodyParts count];
        
    } else if (pickerView == self.distancePicker) {
        
        return sizeof(distances) / sizeof(distances[0]);
        
    } else if (pickerView == self.heightPicker) {
        
        return sizeof(heights) / sizeof(heights[0]);
        
    } else if (pickerView == self.sizePicker) {
        
        return sizeof(sizes) / sizeof(sizes[0]);
        
    } else {
        
        return 0;
    }
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component {
    
    if (pickerView == self.bodyPartPicker) {
        NSString* body_name = [[self.bodyParts objectAtIndex:row] title];
        return [NSString stringWithFormat:@"%@", body_name];
        
    } else if (pickerView == self.distancePicker) {
        
        NSString *dist = [NSNumberFormatter localizedStringFromNumber:@(distances[row]) numberStyle:NSNumberFormatterDecimalStyle];
        
        return [NSString stringWithFormat:@"%@cm", dist];
        
    } else if (pickerView == self.heightPicker) {
        
        NSString *height = [NSNumberFormatter localizedStringFromNumber:@(heights[row]) numberStyle:NSNumberFormatterDecimalStyle];
        
        return [NSString stringWithFormat:@"%@cm", height];
        
    } else if (pickerView == self.sizePicker) {
        
        NSString *width = [NSNumberFormatter localizedStringFromNumber:@(sizes[row].width) numberStyle:NSNumberFormatterDecimalStyle];
        NSString *height = [NSNumberFormatter localizedStringFromNumber:@(sizes[row].height) numberStyle:NSNumberFormatterDecimalStyle];
        
        return [NSString stringWithFormat:@"%@cm x %@cm", width, height];
        
    } else {
        
        return nil;
    }
}

#pragma mark - Picker delegate

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component {
    
    if (pickerView == self.bodyPartPicker) {
        
        self.overlayBodyPart = [self.bodyParts objectAtIndex:row];
        self.overlayBodyPartIndex = (int) row;
        
        [self updateBodyPartCell];
        
    } else if (pickerView == self.distancePicker) {
        
        self.overlayDistance = distances[row];
        
        [self updateDistanceCell];
        
    } else if (pickerView == self.heightPicker) {
        
        self.overlayHeight = heights[row];
        
        [self updateHeightCell];
        
    } else if (pickerView == self.sizePicker) {
        
        self.overlaySize = sizes[row];
        
        [self updateSizeCell];
    }
}

#pragma mark - Notification handlers

- (void)formatChanged:(NSNotification *)notification {
    
    FOVFormatViewController *controller = notification.object;
    
    self.videoFormat = controller.videoFormat;
    
    [self updateFormatCell];
}

- (void)gravityChanged:(NSNotification *)notification {
    
    FOVGravityViewController *controller = notification.object;
    
    self.videoGravity = controller.videoGravity;
    
    [self updateGravityCell];
}

#pragma mark - Helpers

- (void)updateCells {
    
    [self updateFormatCell];
    [self updateGravityCell];
    [self updateBodyPartCell];
    [self updateDistanceCell];
    [self updateHeightCell];
    [self updateSizeCell];
    [self updateLensCells];
    
}



- (void)updateBodyPartCell {

    if (self.overlayBodyPartIndex >= 0) {
        BodyPart* choice =  self.overlayBodyPart;
        if (choice != nil) {
        _overlayDistance = choice.distance;
        _overlayHeight = choice.depth;
        _overlaySize = choice.size;
        
        self.overlayBodyPartIndex =  self.overlayBodyPartIndex % ([self.bodyParts count]);
        NSString* body_name = [[self.bodyParts objectAtIndex:self.overlayBodyPartIndex] title];
        self.bodyPartCell.detailTextLabel.text = body_name;
        }
    }
}

//        [self updateDistanceCell];
//        [self updateHeightCell];
//        [self updateSizeCell];



- (void)updateFormatCell {
    
    if (self.videoFormat.length > 0) {
        
        self.formatCell.detailTextLabel.text = [self.videoFormat stringByReplacingOccurrencesOfString:@"AVCaptureSessionPreset" withString:@""];
        
    } else {
        
        self.formatCell.detailTextLabel.text = NSLocalizedString(@"undefined", nil);
    }
}

- (void)updateGravityCell {
    
    if (self.videoGravity.length > 0) {
        
        self.gravityCell.detailTextLabel.text = [self.videoGravity substringFromIndex:19];
        
    } else {
        
        self.gravityCell.detailTextLabel.text = NSLocalizedString(@"undefined", nil);
    }
}

- (void)updateDistanceCell {
    
    NSString *dist = [NSNumberFormatter localizedStringFromNumber:@(self.overlayDistance) numberStyle:NSNumberFormatterDecimalStyle];
    
    self.distanceCell.detailTextLabel.text = [NSString stringWithFormat:@"%@cm", dist];
}



- (void)updateHeightCell {
    
    NSString *height = [NSNumberFormatter localizedStringFromNumber:@(self.overlayHeight) numberStyle:NSNumberFormatterDecimalStyle];
    
    self.heightCell.detailTextLabel.text = [NSString stringWithFormat:@"%@cm", height];
}

- (void)updateSizeCell {
    
    NSString *width = [NSNumberFormatter localizedStringFromNumber:@(self.overlaySize.width) numberStyle:NSNumberFormatterDecimalStyle];
    NSString *height = [NSNumberFormatter localizedStringFromNumber:@(self.overlaySize.height) numberStyle:NSNumberFormatterDecimalStyle];
    
    self.sizeCell.detailTextLabel.text = [NSString stringWithFormat:@"%@cm x %@cm", width, height];
}

- (void)updateLensCells {
    
    if (self.isLensAdjustmentEnabled) {
        
        self.lensAdjustSwitch.on = YES;
        self.lensAdjustStepper.enabled = YES;
        self.lensAdjustStepper.value = self.lensAdjustmentFactor * 100.0;
        self.lensAdjustLabel.text = [NSString stringWithFormat:@"%@: %@", NSLocalizedString(@"Amount", nil), [NSNumberFormatter localizedStringFromNumber:@(self.lensAdjustmentFactor) numberStyle:NSNumberFormatterPercentStyle]];
        
    } else {
        
        self.lensAdjustSwitch.on = NO;
        self.lensAdjustStepper.enabled = NO;
        self.lensAdjustLabel.text = [NSString stringWithFormat:@"%@:", NSLocalizedString(@"Amount", nil)];
    }
}

@end
